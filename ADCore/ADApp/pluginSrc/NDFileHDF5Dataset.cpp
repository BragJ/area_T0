#include "NDFileHDF5Dataset.h"
#include <iostream>

static const char *fileName = "NDFileHDF5Dataset";

/** Constructor.
 * \param[in] pAsynUser - asynUser that is used to control debugging output
 * \param[in] name - String name of the dataset.
 * \param[in] dataset - HDF5 handle to the dataset.
 */
NDFileHDF5Dataset::NDFileHDF5Dataset(asynUser *pAsynUser, const std::string& name, hid_t dataset) : 
                                     pAsynUser_(pAsynUser), name_(name), dataset_(dataset), nextRecord_(0)
{
  this->maxdims_     = NULL;
  this->chunkdims_   = NULL;
  this->dims_        = NULL;
  this->offset_      = NULL;
  this->virtualdims_ = NULL;
}
 
/** configureDims.
 * Setup any extra dimensions required for this dataset
 * \param[in] pArray - A pointer to an NDArray which contains dimension information.
 * \param[in] multiframe - Is this multiframe?
 * \param[in] extradimensions - The number of extra dimensions.
 * \param[in] extra_dims - The size of extra dimensions.
 * \param[in] user_chunking - Array of user defined chunking dimensions.
 */
asynStatus NDFileHDF5Dataset::configureDims(NDArray *pArray, bool multiframe, int extradimensions, int *extra_dims, int *user_chunking)
{
  int i=0,j=0, extradims = 0, ndims=0;
  asynStatus status = asynSuccess;

  extradims = extradimensions;

  ndims = pArray->ndims + extradims;

  // first check whether the dimension arrays have been allocated
  // or the number of dimensions have changed.
  // If necessary free and reallocate new memory.
  if (this->maxdims_ == NULL || this->rank_ != ndims){
    if (this->maxdims_     != NULL) free(this->maxdims_);
    if (this->chunkdims_   != NULL) free(this->chunkdims_);
    if (this->dims_        != NULL) free(this->dims_);
    if (this->offset_      != NULL) free(this->offset_);
    if (this->virtualdims_ != NULL) free(this->virtualdims_);

    this->maxdims_       = (hsize_t*)calloc(ndims,     sizeof(hsize_t));
    this->chunkdims_     = (hsize_t*)calloc(ndims,     sizeof(hsize_t));
    this->dims_          = (hsize_t*)calloc(ndims,     sizeof(hsize_t));
    this->offset_        = (hsize_t*)calloc(ndims,     sizeof(hsize_t));
    this->virtualdims_   = (hsize_t*)calloc(extradims, sizeof(hsize_t));
  }

  if (multiframe){
    // Configure the virtual dimensions -i.e. dimensions in addition to the frame format.
    // Normally set to just 1 by default or -1 unlimited (in HDF5 terms)
    for (i=0; i<extradims; i++){
      this->chunkdims_[i]   = 1;
      this->maxdims_[i]     = H5S_UNLIMITED;
      this->dims_[i]        = 1;
      this->offset_[i]      = 0; // because we increment offset *before* each write we need to start at -1
      this->virtualdims_[i] = extra_dims[i];
    }
  }

  this->rank_ = ndims;

  for (j=pArray->ndims-1,i=extradims; i<this->rank_; i++,j--){
    this->chunkdims_[i]  = pArray->dims[j].size;
    this->maxdims_[i]    = pArray->dims[j].size;
    this->dims_[i]       = pArray->dims[j].size;
    this->offset_[i]     = 0;
  }

  // Collect the user defined chunking dimensions and check if they're valid
  //
  // A check is made to see if the user has input 0 or negative value (which is invalid)
  // in which case the size of the chunking is set to the maximum size of that dimension (full frame)
  // If the maximum of a particular dimension is set to a negative value -which is the case for
  // infinite lenght dimensions (-1); the chunking value is set to 1.
  int max_items = 0;
  int hdfdim = 0;
  for (i = 0; i<ndims; i++){
    hdfdim = ndims - i - 1;
    max_items = (int)this->maxdims_[hdfdim];
    if (max_items <= 0){
      max_items = 1; // For infinite length dimensions
    } else {
      if (user_chunking[i] > max_items) user_chunking[i] = max_items;
    }
    if (user_chunking[i] < 1) user_chunking[i] = max_items;
    this->chunkdims_[hdfdim] = user_chunking[i];
  }
  return status;
}

/** extendDataSet.
 * Extend this dataset as necessary.  If no extra dimensions are specified
 * then the dataset is simply increased in the frame number direction.
 * \param[in] extradims - The number of extra dimensions.
 */
void NDFileHDF5Dataset::extendDataSet(int extradims)
{
  int i=0;
  bool growdims = true;
  bool growoffset = true;

  // Add the n'th frame dimension (for multiple frames per scan point)
  extradims += 1;

  // first frame already has the offsets and dimensions preconfigured so
  // we dont need to increment anything here
  if (this->nextRecord_ == 0) return;

  // in the simple case where dont use the extra X,Y dimensions we
  // just increment the n'th frame number
  if (extradims == 1){
    this->dims_[0]++;
    this->offset_[0]++;
    return;
  }

  // run through the virtual dimensions in reverse order: n,X,Y
  // and increment, reset or ignore the offset of each dimension.
  for (i=extradims-1; i>=0; i--){
    if (this->dims_[i] == this->virtualdims_[i]) growdims = false;

    if (growoffset){
      this->offset_[i]++;
      growoffset = false;
    }

    if (growdims){
      if (this->dims_[i] < this->virtualdims_[i]) {
        this->dims_[i]++;
        growdims = false;
      }
    }

    if (this->offset_[i] == this->virtualdims_[i]) {
      this->offset_[i] = 0;
      growoffset = true;
      growdims = true;
    }
  }
  return;
}

/** writeFile.
 * Write the data using the HDF5 library calls.
 * \param[in] pArray - The NDArray containing the data to write.
 * \param[in] datatype - The HDF5 datatype of the data.
 * \param[in] dataspace - A handle to the HDF5 dataspace for this dataset.
 * \param[in] framesize - The size of the data to write.
 */
asynStatus NDFileHDF5Dataset::writeFile(NDArray *pArray, hid_t datatype, hid_t dataspace, hsize_t *framesize)
{
  //std::cout<< " it's the NDArrays is ::::::::::::::::"<< pArray->pData <<std::endl; 	
  herr_t hdfstatus;
  static const char *functionName = "writeFile";

  // Increase the size of the dataset
  asynPrint(this->pAsynUser_, ASYN_TRACE_FLOW,
            "%s::%s: set_extent dims={%d,%d,%d}\n",
            fileName, functionName, (int)this->dims_[0], (int)this->dims_[1], (int)this->dims_[2]);

  hdfstatus = H5Dset_extent(this->dataset_, this->dims_);
  if (hdfstatus){
    asynPrint(this->pAsynUser_, ASYN_TRACE_ERROR, 
              "%s::%s ERROR Increasing the size of the dataset [%s] failed\n", 
              fileName, functionName, this->name_.c_str());
    return asynError;
  }
  // Select a hyperslab.
  hid_t fspace = H5Dget_space(this->dataset_);
  if (fspace < 0){
    asynPrint(this->pAsynUser_, ASYN_TRACE_ERROR, 
              "%s::%s ERROR Unable to get a copy of the dataspace for dataset [%s]\n", 
              fileName, functionName, this->name_.c_str());
    return asynError;
  }
  hdfstatus = H5Sselect_hyperslab(fspace, H5S_SELECT_SET, this->offset_, NULL, framesize, NULL);
  if (hdfstatus){
    asynPrint(this->pAsynUser_, ASYN_TRACE_ERROR, 
              "%s::%s ERROR Unable to select hyperslab\n", 
              fileName, functionName);
    return asynError;
  }
  // Write the data to the hyperslab.
  hdfstatus = H5Dwrite(this->dataset_, datatype, dataspace, fspace, H5P_DEFAULT, pArray->pData);
  std::cout<< " it's the NDArrays is ::::::::::::::::"<< pArray->pData <<std::endl; 
  if (hdfstatus){
    asynPrint(this->pAsynUser_, ASYN_TRACE_ERROR, 
              "%s::%s ERROR Unable to write data to hyperslab\n", 
              fileName, functionName);
    return asynError;
  }

  hdfstatus = H5Sclose(fspace);
  if (hdfstatus){
    asynPrint(this->pAsynUser_, ASYN_TRACE_ERROR, 
              "%s::%s ERROR Unable to close the dataspace\n", 
              fileName, functionName);
    return asynError;
  }

  this->nextRecord_++;

  return asynSuccess;
}

/** getHandle.
 * Returns the HDF5 handle to this dataset.
 */
hid_t NDFileHDF5Dataset::getHandle()
{
  return this->dataset_;
}

