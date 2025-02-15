/*=========================================================================
 *
 *  Copyright RTK Consortium
 *
 *  Licensed under the Apache License, Version 2.0 (the "License");
 *  you may not use this file except in compliance with the License.
 *  You may obtain a copy of the License at
 *
 *         http://www.apache.org/licenses/LICENSE-2.0.txt
 *
 *  Unless required by applicable law or agreed to in writing, software
 *  distributed under the License is distributed on an "AS IS" BASIS,
 *  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *  See the License for the specific language governing permissions and
 *  limitations under the License.
 *
 *=========================================================================*/

#ifndef rtkProjectionGeometry_h
#define rtkProjectionGeometry_h

#include <itkImageBase.h>

#include <vector>

#include "rtkMacro.h"

namespace rtk
{

/** \class ProjectionGeometry
 * \brief A templated class holding a vector of M x (M+1) matrices
 *
 * This class contains a vector of projection matrices.
 * Each matrix corresponds to a different position of a
 * projector, e.g. a detector and an x-ray source.
 * The class is meant to be specialized for specific geometries.
 *
 * \author Simon Rit
 *
 * \ingroup RTK Geometry
 */
template <unsigned int TDimension = 3>
class ITK_TEMPLATE_EXPORT ProjectionGeometry : public itk::DataObject
{
public:
  ITK_DISALLOW_COPY_AND_MOVE(ProjectionGeometry);

  using Self = ProjectionGeometry<TDimension>;
  using Superclass = itk::DataObject;
  using Pointer = itk::SmartPointer<Self>;
  using ConstPointer = itk::SmartPointer<const Self>;

  /** Method for creation through the object factory. */
  itkNewMacro(Self);

  /** Convenient type alias */
  using SizeType = typename itk::ImageBase<TDimension>::SizeType;
  using PointType = typename itk::ImageBase<TDimension>::PointType;
  using SpacingType = typename itk::ImageBase<TDimension>::SpacingType;

  using MatrixType = typename itk::Matrix<double, TDimension, TDimension + 1>;

  /** Get the vector of projection matrices.
   * A projection matrix is a M x (M+1) homogeneous matrix.
   * The multiplication of a M-D point in physical coordinates
   * with the i-th matrix provides the physical coordinate on
   * the i-th projection.
   */
  const std::vector<MatrixType> &
  GetMatrices() const
  {
    return this->m_Matrices;
  }

  /** Get the i-th projection matrix. */
  MatrixType
  GetMatrix(const unsigned int i) const
  {
    if (i >= this->m_Matrices.size())
    {
      itkExceptionMacro(<< "Requested matrix index " << i << " is out of bound.");
    }
    return this->m_Matrices[i];
  }

  /** Empty the geometry object. */
  virtual void
  Clear();

protected:
  ProjectionGeometry() = default;
  ~ProjectionGeometry() override = default;

  void
  PrintSelf(std::ostream & os, itk::Indent indent) const override;

  /** Add projection matrix */
  virtual void
  AddMatrix(const MatrixType & m)
  {
    this->m_Matrices.push_back(m);
    this->Modified();
  }

private:
  /** Projection matrices */
  std::vector<MatrixType> m_Matrices;
};
} // namespace rtk

#include "rtkProjectionGeometry.hxx"

#endif // rtkProjectionGeometry_h
