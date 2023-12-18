:: create the conda environment as a subdirectory
call mamba env create -p .conda/freecad -f conda/conda-env.yaml

:: add the environment subdirectory to the conda configuration
call conda config --add envs_dirs %CONDA_PREFIX%/envs
call conda config --add envs_dirs %CD%/.conda
call conda config --set env_prompt "({name})"

:: install the FreeCAD dependencies into the environment
call mamba run --live-stream -n freecad mamba-devenv --no-prune -f conda/environment.devenv.yml
