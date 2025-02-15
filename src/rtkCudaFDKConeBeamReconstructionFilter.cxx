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

#include "rtkCudaFDKConeBeamReconstructionFilter.h"

rtk::CudaFDKConeBeamReconstructionFilter ::CudaFDKConeBeamReconstructionFilter()
{
  // Create each filter which are specific for cuda
  m_WeightFilter = WeightFilterType::New();
  m_RampFilter = RampFilterType::New();
  m_BackProjectionFilter = BackProjectionFilterType::New();

  // Permanent internal connections
  m_WeightFilter->SetInput(m_ExtractFilter->GetOutput());
  m_RampFilter->SetInput(m_WeightFilter->GetOutput());
  m_BackProjectionFilter->SetInput(1, m_RampFilter->GetOutput());

  // Default parameters
  m_BackProjectionFilter->InPlaceOn();
  m_BackProjectionFilter->SetTranspose(false);

  // Overwrite subset size of CPU version with GPU default
  CPUSuperclass::SetProjectionSubsetSize(SLAB_SIZE);
}

void
rtk::CudaFDKConeBeamReconstructionFilter ::GPUGenerateData()
{
  CPUSuperclass::GenerateData();
}
