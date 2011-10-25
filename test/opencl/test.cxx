#include <vigra/multi_array.hxx>
#include <vigra/multi_opencl.hxx>

int main(int argc, char *argv[])
{
  MultiArray<3u, cl_float3>::difference_type shape(3,4,5);
  MultiArray<3u, cl_float3> A, B, C;
  MultiArray<3u, float> d(MultiArray<3u, float>::difference_type(3, 4, 5);

  // setup some content
  for (int i = 0; i < 3; i++)
    for (int j = 0; j < 4; j++)
      for (int k = 0; k < 5; k++)
        A(i, j, k).x = A(i,j,k).y = A(i,j,k).z = i+j+k;

  // unary ops, scalar result, element-wise (6.11.5)
  d = length(A); // d-entries get the length of all the A entries (= sqrt(i^2 + j^2 + k^2)
  d = normalize(A);

  // unary ops, vector result, element-wise (6.3)
  B = A + C;
  A = B / C;

  // binary ops, scalar result (ie. 6.11.5)
  d = distance(A, B);
  d = dot(A, B);

  // binary comparison, element-wise (6.11.6, but with c++ syntax, or OpenCL function names)
  d = A > B; d = isgreater(A, B);
}
