# -*- coding: utf-8 -*-
# @Author: Haozhe Xie
# @Date:   2019-08-07 20:54:24
# @Last Modified by:   Haozhe Xie
# @Last Modified time: 2019-12-10 10:04:25
# @Email:  cshzxie@gmail.com

from setuptools import setup
from torch.utils.cpp_extension import BuildExtension, CUDAExtension

setup(name='chamfer',
      version='2.0.0',
      ext_modules=[
          CUDAExtension('chamfer', [
              'chamfer_cuda.cpp',
              'chamfer.cu',
          ]),
      ],
      cmdclass={'build_ext': BuildExtension})
