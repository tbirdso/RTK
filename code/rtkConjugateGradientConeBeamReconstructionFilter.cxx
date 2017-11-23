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

#include "rtkConjugateGradientConeBeamReconstructionFilter.h"

namespace rtk
{

template<>
ConjugateGradientConeBeamReconstructionFilter< itk::VectorImage<double, 3>, itk::Image<double, 3> >
::ConjugateGradientConeBeamReconstructionFilter()
{
  this->SetNumberOfRequiredInputs(3);

  // Set the default values of member parameters
  m_NumberOfIterations=3;
  m_MeasureExecutionTimes=false;
  m_IterationCosts=false;

  m_Gamma = 0;
  m_Tikhonov = 0;
  m_Regularized = false;
  m_CudaConjugateGradient = true;
  m_DisableDisplacedDetectorFilter = false;
  m_TargetSumOfSquaresBetweenConsecutiveIterates = 0;

  // Create the filters
#ifdef RTK_USE_CUDA
  m_DisplacedDetectorFilter = rtk::CudaDisplacedDetectorImageFilter::New();
  m_ConstantVolumeSource     = rtk::CudaConstantVolumeSource::New();
#else
  m_DisplacedDetectorFilter = DisplacedDetectorFilterType::New();
  m_ConstantVolumeSource     = ConstantImageSourceType::New();
#endif
  m_CGOperator = CGOperatorFilterType::New();

  m_MultiplyVolumeFilter = MultiplyFilterType::New();
  m_MatrixVectorMultiplyFilter = MatrixVectorMultiplyFilterType::New();
  m_MultiplyOutputFilter = MultiplyFilterType::New();

  // Set permanent parameters
  m_ConstantVolumeSource->SetConstant(0);
  m_DisplacedDetectorFilter->SetPadOnTruncatedSide(false);
}

template<>
void
ConjugateGradientConeBeamReconstructionFilter< itk::VectorImage<double, 3>, itk::Image<double, 3> >
::GenerateOutputInformation()
{
  // Choose between cuda or non-cuda conjugate gradient filter
  m_ConjugateGradientFilter = ConjugateGradientFilterType::New();
#ifdef RTK_USE_CUDA
  if (m_CudaConjugateGradient)
    m_ConjugateGradientFilter = rtk::CudaConjugateGradientImageFilter_3f::New();
#endif
  m_ConjugateGradientFilter->SetA(m_CGOperator.GetPointer());
  m_ConjugateGradientFilter->SetTargetSumOfSquaresBetweenConsecutiveIterates(m_TargetSumOfSquaresBetweenConsecutiveIterates);
  m_ConjugateGradientFilter->SetIterationCosts(m_IterationCosts);

  // Set runtime connections
  m_ConstantVolumeSource->SetInformationFromImage(this->GetInput(0));
  m_CGOperator->SetInput(1, this->GetInput(1));
  m_CGOperator->SetSupportMask(this->GetSupportMask());
  m_ConjugateGradientFilter->SetX(this->GetInput(0));
  m_DisplacedDetectorFilter->SetDisable(m_DisableDisplacedDetectorFilter);
  m_DisplacedDetectorFilter->SetInput(this->GetInput(2));

  // Links with the m_BackProjectionFilter should be set here and not
  // in the constructor, as m_BackProjectionFilter is set at runtime
  m_BackProjectionFilterForB->SetInput(0, m_ConstantVolumeSource->GetOutput());
  m_ConjugateGradientFilter->SetB(m_BackProjectionFilterForB->GetOutput());

  // Set the matrix vector multiply filter's inputs for multiplication
  // by the inverse covariance matrix (for GLS minimization)
  m_MatrixVectorMultiplyFilter->SetInput1(this->GetInput(1));
  m_MatrixVectorMultiplyFilter->SetInput2(m_DisplacedDetectorFilter->GetOutput());
  m_CGOperator->SetInput(2, m_DisplacedDetectorFilter->GetOutput());
  m_BackProjectionFilterForB->SetInput(1, m_MatrixVectorMultiplyFilter->GetOutput());

  // If a support mask is used, it serves as preconditioning weights
  if (this->GetSupportMask().IsNotNull())
    {
    // Multiply the volume by support mask, and pass it to the conjugate gradient operator
    m_MultiplyVolumeFilter->SetInput1(m_BackProjectionFilterForB->GetOutput());
    m_MultiplyVolumeFilter->SetInput2(this->GetSupportMask());
    m_CGOperator->SetSupportMask(this->GetSupportMask());
    m_ConjugateGradientFilter->SetB(m_MultiplyVolumeFilter->GetOutput());

    // Multiply the output by the support mask
    m_MultiplyOutputFilter->SetInput1(m_ConjugateGradientFilter->GetOutput());
    m_MultiplyOutputFilter->SetInput2(this->GetSupportMask());
    }

  // For the same reason, set geometry now
  m_CGOperator->SetGeometry(this->m_Geometry);
  m_BackProjectionFilterForB->SetGeometry(this->m_Geometry.GetPointer());
  m_DisplacedDetectorFilter->SetGeometry(this->m_Geometry);

  // Set runtime parameters
  m_ConjugateGradientFilter->SetNumberOfIterations(this->m_NumberOfIterations);
  m_CGOperator->SetGamma(m_Gamma);
  m_CGOperator->SetTikhonov(m_Tikhonov);

  // Set memory management parameters
  m_MatrixVectorMultiplyFilter->ReleaseDataFlagOn();
  m_BackProjectionFilterForB->ReleaseDataFlagOn();
  if (this->GetSupportMask().IsNotNull())
    {
    m_MultiplyVolumeFilter->ReleaseDataFlagOn();
    m_MultiplyOutputFilter->ReleaseDataFlagOn();
    }

  // Have the last filter calculate its output information
  m_ConjugateGradientFilter->UpdateOutputInformation();

  // Copy it as the output information of the composite filter
  this->GetOutput()->CopyInformation( m_ConjugateGradientFilter->GetOutput() );
}

template<>
void
ConjugateGradientConeBeamReconstructionFilter< itk::VectorImage<double, 3>, itk::Image<double, 3> >
::GenerateData()
{
  itk::TimeProbe ConjugateGradientTimeProbe;
  if(m_IterationCosts)
    {
    typename DotProductFilterType::Pointer dotProduct = DotProductFilterType::New();
    dotProduct->SetInput(0, m_MatrixVectorMultiplyFilter->GetOutput());
    dotProduct->SetInput(1, this->GetInput(1));
    typename StatisticsFilterType::Pointer stats = StatisticsFilterType::New();
    stats->SetInput(dotProduct->GetOutput());
    stats->Update();
    m_ConjugateGradientFilter->SetC(0.5*stats->GetSum());
    }

  if(m_MeasureExecutionTimes)
    {
    std::cout << "Starting ConjugateGradient" << std::endl;
    ConjugateGradientTimeProbe.Start();
    }

  m_ConjugateGradientFilter->Update();

  if (this->GetSupportMask())
    {
    m_MultiplyOutputFilter->Update();
    }

  if(m_MeasureExecutionTimes)
    {
    ConjugateGradientTimeProbe.Stop();
    std::cout << "ConjugateGradient took " << ConjugateGradientTimeProbe.GetTotal() << ' ' << ConjugateGradientTimeProbe.GetUnit() << std::endl;
    }

  if (this->GetSupportMask())
    {
    this->GraftOutput( m_MultiplyOutputFilter->GetOutput() );
    }
    else
    {
    this->GraftOutput( m_ConjugateGradientFilter->GetOutput() );
    }
}

} // end namespace rtk
