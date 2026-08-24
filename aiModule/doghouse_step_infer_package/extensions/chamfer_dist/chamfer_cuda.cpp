/*
 * @Author: Haozhe Xie
 * @Date:   2019-08-07 20:54:24
 * @Last Modified by:   Haozhe Xie
 * @Last Modified time: 2019-12-10 10:33:50
 * @Email:  cshzxie@gmail.com
 */

#include <torch/extension.h>
#include <vector>

std::vector<torch::Tensor> chamfer_cuda_forward(torch::Tensor xyz1,
                                                torch::Tensor xyz2);

std::vector<torch::Tensor> chamfer_cuda_backward(torch::Tensor xyz1,
                                                 torch::Tensor xyz2,
                                                 torch::Tensor idx1,
                                                 torch::Tensor idx2,
                                                 torch::Tensor grad_dist1,
                                                 torch::Tensor grad_dist2);

std::vector<torch::Tensor> chamfer_forward(torch::Tensor xyz1,
                                           torch::Tensor xyz2) {
  return chamfer_cuda_forward(xyz1, xyz2);
}

std::vector<torch::Tensor> chamfer_backward(torch::Tensor xyz1,
                                            torch::Tensor xyz2,
                                            torch::Tensor idx1,
                                            torch::Tensor idx2,
                                            torch::Tensor grad_dist1,
                                            torch::Tensor grad_dist2) {
  return chamfer_cuda_backward(xyz1, xyz2, idx1, idx2, grad_dist1, grad_dist2);
}

PYBIND11_MODULE(TORCH_EXTENSION_NAME, m) {
  m.def("forward", &chamfer_forward, "Chamfer forward (CUDA)");
  m.def("backward", &chamfer_backward, "Chamfer backward (CUDA)");
}
