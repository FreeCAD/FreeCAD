# AutoMate
Dataset and code for the automatic mating of CAD assemblies.

## Paper
Please see our [paper](https://dl.acm.org/doi/10.1145/3478513.3480562) for more details.

## Dataset
The AutoMate Dataset can be downloaded from [Zenodo](https://zenodo.org/record/7776208#.ZDcYinbMIQ8).


## Installation

Automate relies on C++ extension modules to read Parasolid and STEP files. Since Parasolid is a proprietary CAD kernel, we can't distribute it, so you need to have the distribution on your machine already and compile it at install time.

### Requirements

Installation relies on CMake, OpenCascade, and Parasolid. In the future, we intend to make the Parasolid dependency optional. The easiest way to get the first two dependencies (and all python dependencies) is to install the conda environments `environment.yml` or `minimal_env.yml`:

`conda env create -f [environment|minimal_env].yml`

The Parasolid requirement relies on setting the environmental variable `$PARASOLID_BASE` on your system pointing to the Parasolid install directory for your operating system. For example

``export PARASOLID_BASE=${PATH_TO_PARASOLID_INSTALL}/intel_linux/base``

Replace ``intel_linux`` with the directory appropriate to your OS. The base directory should contain files like `pskernel_archive.lib` and `parasolid_kernel.h`.

Once these requirements are met, you an install via pip:

`pip install git+https://github.com/degravity/automate.git@v1.0.4`


### Troubleshooting

```
ImportError: dynamic module does not define module export function (PyInit_automate_cpp)
```

If you get this error when trying to import part of the module, it means that python can't find the C++ extensions module. To fix this, try cleaning out **all** build files and building again.

## Citing

If you use this code our the [AutoMate Dataset](https://zenodo.org/record/7776208#.ZDcYinbMIQ8) in your work, please cite us:

```
@article{10.1145/3478513.3480562,
author = {Jones, Benjamin and Hildreth, Dalton and Chen, Duowen and Baran, Ilya and Kim, Vladimir G. and Schulz, Adriana},
title = {AutoMate: A Dataset and Learning Approach for Automatic Mating of CAD Assemblies},
year = {2021},
issue_date = {December 2021},
publisher = {Association for Computing Machinery},
address = {New York, NY, USA},
volume = {40},
number = {6},
issn = {0730-0301},
url = {https://doi.org/10.1145/3478513.3480562},
doi = {10.1145/3478513.3480562},
month = {dec},
articleno = {227},
numpages = {18},
keywords = {assembly-based modeling, representation learning, boundary representation, computer-aided design}
}
```
