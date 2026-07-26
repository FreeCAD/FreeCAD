from argparse import ArgumentParser
import sys
import os
import importlib
import json
from .arg_parsing import from_argparse_args, add_argparse_args
import pytorch_lightning as pl
from pytorch_lightning.loggers import TensorBoardLogger

class ArgparseInitialized:
    @classmethod
    def from_argparse_args(cls, args, **kwargs):
        return from_argparse_args(cls, args, **kwargs)
    
    @classmethod
    def add_argparse_args(cls, parent_parser):
        return add_argparse_args(cls, parent_parser)

def fix_file_descriptors():
    # When run in a tmux environment, the file_descriptor strategy can run out
    # of file handles quickly unless you reset the file handle limit
    if sys.platform == "linux" or sys.platform == "linux2":
        import resource
        from torch.multiprocessing import set_sharing_strategy
        set_sharing_strategy("file_descriptor")
        resource.setrlimit(resource.RLIMIT_NOFILE, (100_000, 100_000))

def get_class(classname: str):
    parts = classname.split('.')
    module_name = '.'.join(parts[:-1])
    class_name = parts[-1]
    module = importlib.import_module(module_name)
    return getattr(module, class_name)

# TODOs: deterministic seeding by default
#        
def run_model(default_args = dict()):
    fix_file_descriptors()
    parser = ArgumentParser(allow_abbrev=False, conflict_handler='resolve')

    parser.add_argument('--argfile', type=str, default=None)
    parser.add_argument('--model_class', type=str, default=None)
    parser.add_argument('--data_class', type=str, default=None)

    args, _ = parser.parse_known_args()
    if args.argfile is not None:
        if not os.path.exists(args.argfile):
            print(f'Could not find argfile: {args.argfile}')
            exit()
        with open(args.argfile, 'r') as argfile:
            file_args = json.load(argfile)
            for key in file_args:
                default_args[key] = file_args[key]
        default_args['name'] = os.path.splitext(os.path.split(args.argfile)[1])[0]
    if 'model_class' in default_args and args.model_class is None:
        args.model_class = default_args['model_class']
    if 'data_class' in default_args and args.data_class is None:
        args.data_class = default_args['data_class']
    
    if args.model_class is None or args.data_class is None:
        print('You must specify a --model_class and a --data_class')
        exit()
    
    model_cls = get_class(args.model_class)
    data_module_cls = get_class(args.data_class)

    parser = model_cls.add_argparse_args(parser)
    parser = data_module_cls.add_argparse_args(parser)
    parser = pl.Trainer.add_argparse_args(parser)
    
    parser.add_argument('--tensorboard_path', type=str, default='.')
    parser.add_argument('--checkpoint_path', type=str, default=None)
    parser.add_argument('--name', type=str, default='unnamed')
    parser.add_argument('--debug', action='store_true')
    parser.add_argument('--resume_version', type=int, default=None)
    parser.add_argument('--seed', type=int, default=None)
    parser.add_argument('--no_train', action='store_true')
    parser.add_argument('--no_test', action='store_true')

    parser.set_defaults(**default_args)

    args = parser.parse_args()

    if args.seed is not None:
        pl.seed_everything(args.seed)
        args.deterministic = True

    #logger = None if args.debug else TensorBoardLogger(
    logger = TensorBoardLogger(
        args.tensorboard_path,
        name=args.name,
        default_hp_metric = False,
        version=args.resume_version
    )
    logger.log_hyperparams(args)

    if not args.debug and args.resume_version is not None and \
            args.checkpoint_path is not None and args.resume_chackpoint is None:
        last_ckpt = os.path.join(
            os.path.dirname(logger.experiement.log_dir),
            'checkpoints',
            'last.ckpt'
        )
        if not os.path.exists(last_ckpt):
            print(f'No last checkpoint found for version_{args.resume_version}.')
            print(f'Tried {last_ckpt}')
            exit()
        args.checkpoint_path = last_ckpt
        args.resume_from_checkpoint = last_ckpt
    
    data = data_module_cls.from_argparse_args(args)
    if args.checkpoint_path is None:
        model = model_cls.from_argparse_args(args)
    else:
        model = model_cls.load_from_checkpoint(args.checkpoint_path)
    
    callbacks = model.get_callbacks()

    trainer = pl.Trainer.from_argparse_args(args, logger=logger, callbacks=callbacks)

    if args.auto_lr_find or args.auto_scale_batch_size:
        trainer.tune(model, datamodule=data)
    
    if not args.no_train:
        trainer.fit(model, data)
    if not args.no_test:
        if args.no_train:
            if args.checkpoint_path is None:
                print('Testing from initialization.')
            else:
                print(f'Testing from {args.checkpoint_path}')
            results = trainer.test(model, datamodule=data)
        else:
            ckpt = trainer.checkpoint_callback.best_model_path
            print(f'Testing from {ckpt}')
            results = trainer.test(datamodule=data)