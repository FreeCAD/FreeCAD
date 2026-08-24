/*
 * @Author: Haozhe Xie
 * @Date:   2019-08-07 20:54:24
 * @Last Modified by:   Haozhe Xie
 * @Last Modified time: 2020-06-17 14:58:55
 * @Email:  cshzxie@gmail.com
 */

#include <cuda.h>
#include <cuda_runtime.h>
#include <torch/extension.h>

#include <vector>

__global__ void chamfer_dist_kernel(int batch_size,
                                    int n,
                                    const float* xyz1,
                                    int m,
                                    const float* xyz2,
                                    float* dist,
                                    int* indexes) {
  const int batch = 512;
  __shared__ float buf[batch * 3];
  for (int i = blockIdx.x; i < batch_size; i += gridDim.x) {
    for (int k2 = 0; k2 < m; k2 += batch) {
      int end_k = min(m, k2 + batch) - k2;
      for (int j = threadIdx.x; j < end_k * 3; j += blockDim.x) {
        buf[j] = xyz2[(i * m + k2) * 3 + j];
      }
      __syncthreads();
      for (int j = threadIdx.x + blockIdx.y * blockDim.x; j < n;
           j += blockDim.x * gridDim.y) {
        float x1            = xyz1[(i * n + j) * 3 + 0];
        float y1            = xyz1[(i * n + j) * 3 + 1];
        float z1            = xyz1[(i * n + j) * 3 + 2];
        float best_dist     = 0;
        int best_dist_index = 0;
        int end_ka          = end_k - (end_k & 3);
        if (end_ka == batch) {
          for (int k = 0; k < batch; k += 4) {
            {
              float x2   = buf[k * 3 + 0] - x1;
              float y2   = buf[k * 3 + 1] - y1;
              float z2   = buf[k * 3 + 2] - z1;
              float dist = x2 * x2 + y2 * y2 + z2 * z2;

              if (k == 0 || dist < best_dist) {
                best_dist       = dist;
                best_dist_index = k + k2;
              }
            }
            {
              float x2   = buf[k * 3 + 3] - x1;
              float y2   = buf[k * 3 + 4] - y1;
              float z2   = buf[k * 3 + 5] - z1;
              float dist = x2 * x2 + y2 * y2 + z2 * z2;
              if (dist < best_dist) {
                best_dist       = dist;
                best_dist_index = k + k2 + 1;
              }
            }
            {
              float x2   = buf[k * 3 + 6] - x1;
              float y2   = buf[k * 3 + 7] - y1;
              float z2   = buf[k * 3 + 8] - z1;
              float dist = x2 * x2 + y2 * y2 + z2 * z2;
              if (dist < best_dist) {
                best_dist       = dist;
                best_dist_index = k + k2 + 2;
              }
            }
            {
              float x2   = buf[k * 3 + 9] - x1;
              float y2   = buf[k * 3 + 10] - y1;
              float z2   = buf[k * 3 + 11] - z1;
              float dist = x2 * x2 + y2 * y2 + z2 * z2;
              if (dist < best_dist) {
                best_dist       = dist;
                best_dist_index = k + k2 + 3;
              }
            }
          }
        } else {
          for (int k = 0; k < end_ka; k += 4) {
            {
              float x2   = buf[k * 3 + 0] - x1;
              float y2   = buf[k * 3 + 1] - y1;
              float z2   = buf[k * 3 + 2] - z1;
              float dist = x2 * x2 + y2 * y2 + z2 * z2;
              if (k == 0 || dist < best_dist) {
                best_dist       = dist;
                best_dist_index = k + k2;
              }
            }
            {
              float x2   = buf[k * 3 + 3] - x1;
              float y2   = buf[k * 3 + 4] - y1;
              float z2   = buf[k * 3 + 5] - z1;
              float dist = x2 * x2 + y2 * y2 + z2 * z2;
              if (dist < best_dist) {
                best_dist       = dist;
                best_dist_index = k + k2 + 1;
              }
            }
            {
              float x2   = buf[k * 3 + 6] - x1;
              float y2   = buf[k * 3 + 7] - y1;
              float z2   = buf[k * 3 + 8] - z1;
              float dist = x2 * x2 + y2 * y2 + z2 * z2;
              if (dist < best_dist) {
                best_dist       = dist;
                best_dist_index = k + k2 + 2;
              }
            }
            {
              float x2   = buf[k * 3 + 9] - x1;
              float y2   = buf[k * 3 + 10] - y1;
              float z2   = buf[k * 3 + 11] - z1;
              float dist = x2 * x2 + y2 * y2 + z2 * z2;
              if (dist < best_dist) {
                best_dist       = dist;
                best_dist_index = k + k2 + 3;
              }
            }
          }
        }
        for (int k = end_ka; k < end_k; k++) {
          float x2   = buf[k * 3 + 0] - x1;
          float y2   = buf[k * 3 + 1] - y1;
          float z2   = buf[k * 3 + 2] - z1;
          float dist = x2 * x2 + y2 * y2 + z2 * z2;
          if (k == 0 || dist < best_dist) {
            best_dist       = dist;
            best_dist_index = k + k2;
          }
        }
        if (k2 == 0 || dist[(i * n + j)] > best_dist) {
          dist[(i * n + j)]    = best_dist;
          indexes[(i * n + j)] = best_dist_index;
        }
      }
      __syncthreads();
    }
  }
}

std::vector<torch::Tensor> chamfer_cuda_forward(torch::Tensor xyz1,
                                                torch::Tensor xyz2) {
  const int batch_size = xyz1.size(0);
  const int n          = xyz1.size(1);  // num_points point cloud A
  const int m          = xyz2.size(1);  // num_points point cloud B
  torch::Tensor dist1 =
    torch::zeros({batch_size, n}, torch::CUDA(torch::kFloat));
  torch::Tensor dist2 =
    torch::zeros({batch_size, m}, torch::CUDA(torch::kFloat));
  torch::Tensor idx1 = torch::zeros({batch_size, n}, torch::CUDA(torch::kInt));
  torch::Tensor idx2 = torch::zeros({batch_size, m}, torch::CUDA(torch::kInt));

  chamfer_dist_kernel<<<dim3(32, 16, 1), 512>>>(
    batch_size, n, xyz1.data_ptr<float>(), m, xyz2.data_ptr<float>(),
    dist1.data_ptr<float>(), idx1.data_ptr<int>());
  chamfer_dist_kernel<<<dim3(32, 16, 1), 512>>>(
    batch_size, m, xyz2.data_ptr<float>(), n, xyz1.data_ptr<float>(),
    dist2.data_ptr<float>(), idx2.data_ptr<int>());

  cudaError_t err = cudaGetLastError();
  if (err != cudaSuccess) {
    printf("Error in chamfer_cuda_forward: %s\n", cudaGetErrorString(err));
  }
  return {dist1, dist2, idx1, idx2};
}

__global__ void chamfer_dist_grad_kernel(int b,
                                         int n,
                                         const float* xyz1,
                                         int m,
                                         const float* xyz2,
                                         const float* grad_dist1,
                                         const int* idx1,
                                         float* grad_xyz1,
                                         float* grad_xyz2) {
  for (int i = blockIdx.x; i < b; i += gridDim.x) {
    for (int j = threadIdx.x + blockIdx.y * blockDim.x; j < n;
         j += blockDim.x * gridDim.y) {
      float x1 = xyz1[(i * n + j) * 3 + 0];
      float y1 = xyz1[(i * n + j) * 3 + 1];
      float z1 = xyz1[(i * n + j) * 3 + 2];
      int j2   = idx1[i * n + j];
      float x2 = xyz2[(i * m + j2) * 3 + 0];
      float y2 = xyz2[(i * m + j2) * 3 + 1];
      float z2 = xyz2[(i * m + j2) * 3 + 2];
      float g  = grad_dist1[i * n + j] * 2;
      atomicAdd(&(grad_xyz1[(i * n + j) * 3 + 0]), g * (x1 - x2));
      atomicAdd(&(grad_xyz1[(i * n + j) * 3 + 1]), g * (y1 - y2));
      atomicAdd(&(grad_xyz1[(i * n + j) * 3 + 2]), g * (z1 - z2));
      atomicAdd(&(grad_xyz2[(i * m + j2) * 3 + 0]), -(g * (x1 - x2)));
      atomicAdd(&(grad_xyz2[(i * m + j2) * 3 + 1]), -(g * (y1 - y2)));
      atomicAdd(&(grad_xyz2[(i * m + j2) * 3 + 2]), -(g * (z1 - z2)));
    }
  }
}

std::vector<torch::Tensor> chamfer_cuda_backward(torch::Tensor xyz1,
                                                 torch::Tensor xyz2,
                                                 torch::Tensor idx1,
                                                 torch::Tensor idx2,
                                                 torch::Tensor grad_dist1,
                                                 torch::Tensor grad_dist2) {
  const int batch_size    = xyz1.size(0);
  const int n             = xyz1.size(1);  // num_points point cloud A
  const int m             = xyz2.size(1);  // num_points point cloud B
  torch::Tensor grad_xyz1 = torch::zeros_like(xyz1, torch::CUDA(torch::kFloat));
  torch::Tensor grad_xyz2 = torch::zeros_like(xyz2, torch::CUDA(torch::kFloat));

  chamfer_dist_grad_kernel<<<dim3(1, 16, 1), 256>>>(
    batch_size, n, xyz1.data_ptr<float>(), m, xyz2.data_ptr<float>(),
    grad_dist1.data_ptr<float>(), idx1.data_ptr<int>(),
    grad_xyz1.data_ptr<float>(), grad_xyz2.data_ptr<float>());
  chamfer_dist_grad_kernel<<<dim3(1, 16, 1), 256>>>(
    batch_size, m, xyz2.data_ptr<float>(), n, xyz1.data_ptr<float>(),
    grad_dist2.data_ptr<float>(), idx2.data_ptr<int>(),
    grad_xyz2.data_ptr<float>(), grad_xyz1.data_ptr<float>());

  cudaError_t err = cudaGetLastError();
  if (err != cudaSuccess) {
    printf("Error in chamfer_cuda_backward: %s\n", cudaGetErrorString(err));
  }
  return {grad_xyz1, grad_xyz2};
}
