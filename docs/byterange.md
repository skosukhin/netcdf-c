NetCDF Byterange Support {#netcdf_byterange}
================================

[TOC]
<!-- Note that this file has the .dox extension, but is mostly markdown -->
<!-- Begin MarkDown -->

# Introduction {#byterange_intro}

Suppose that you have the URL to a remote dataset
which is a normal netcdf-3 or netcdf-4 file.

The netCDF-c library now supports read-only access to such
datasets using the HTTP byte range capability [], assuming that
the remote server supports byte-range access.

Two examples:

1. A Thredds server supporting the Thredds "fileserver" Thredds protocol, and containing a netcdf classic file.
   - location: "https://remotetest.unidata.ucar.edu/thredds/fileserver/testdata/2004050300_eta_211.nc#mode=bytes" 
2. An Amazon S3 dataset containing a netcdf enhanced file.
   - location: "http://noaa-goes16.s3.amazonaws.com/ABI-L1b-RadC/2017/059/03/OR_ABI-L1b-RadC-M3C13_G16_s20170590337505_e20170590340289_c20170590340316.nc#mode=bytes"

Other remote servers may also provide byte-range access in a similar form.

It is important to note that this is not intended as a true
production capability because it is believed that this kind of access
can be quite slow. In addition, the byte-range IO drivers do not
currently do any sort of optimization or caching.

# Configuration {#byterange_config}

This capability is enabled using the option *--enable-byterange* option
to the *./configure* command for Automake. For Cmake, the option flag is
*-DENABLE_BYTERANGE=true*.

This capability requires access to *libcurl*, and an error will occur
if byterange is enabled, but *libcurl* could not be located.
In this, it is similar to the DAP2 and DAP4 capabilities.

# Run-time Usage {#byterange_url}

In order to use this capability at run-time, with *ncdump* for
example, it is necessary to provide a URL pointing to the basic
dataset to be accessed. The URL must be annotated to tell the
netcdf-c library that byte-range access should be used. This is
indicated by appending the phrase ````#mode=bytes````
to the end of the URL.
The two examples above show how this will look.

In order to determine the kind of file being accessed, the
netcdf-c library will read what is called the "magic number"
from the beginning of the remote dataset. This magic number
is a specific set of bytes that indicates the kind of file:
classic, enhanced, cdf5, etc. 

# Architecture {#byterange_arch}

Internally, this capability is implemented with the following drivers:

1. libdispatch/dhttp.c -- wrap libcurl operations.
2. libsrc/httpio.c -- provide byte-range reading to the netcdf-3 dispatcher.
3. libhdf5/H5FDhttp.c -- provide byte-range reading to the netcdf-4 dispatcher for non-cloud storage.
4. H5FDros3.c -- provide byte-range reading to the netcdf-4 dispatcher for cloud storage (Amazon S3 currently).

Both *httpio.c* and *H5FDhttp.c* are adapters that use *dhttp.c*
to do the work. Testing for the magic number is also carried out
by using the *dhttp.c* code.
*H5FDros3* is also an adapter, but specialized for cloud storage access.

## NetCDF Classic Access

The netcdf-3 code in the directory *libsrc* is built using
a secondary dispatch mechanism called *ncio*. This allows the
netcdf-3 code be independent of the lowest level IO access mechanisms.
This is how in-memory and mmap based access is implemented.
The file *httpio.c* is the dispatcher used to provide byte-range
IO for the netcdf-3 code.
Note that *httpio.c* is mostly just an
adapter between the *ncio* API and the *dhttp.c* code.

## NetCDF Enhanced Access

### Non-Cloud Access
Similar to the netcdf-3 code, the HDF5 library
provides a secondary dispatch mechanism *H5FD*. This allows the
HDF5 code to be independent of the lowest level IO access mechanisms.
The netcdf-4 code in libhdf5 is built on the HDF5 library, so
it indirectly inherits the H5FD mechanism.

The file *H5FDhttp.c* implements the H5FD dispatcher API
and provides byte-range IO for the netcdf-4 code 
(and for the HDF5 library as a side effect).
It only works for non-cloud servers such as the Unidata Thredds server.

Note that *H5FDhttp.c* is mostly just an
adapter between the *H5FD* API and the *dhttp.c* code.

#### The dhttp.c Code {#byterange_dhttp}

The core of all this is *dhttp.c* (and its header
*include/nchttp.c*). It is a wrapper over *libcurl*
and so exposes the libcurl handles -- albeit as _void*_.

The API for *dhttp.c* consists of the following procedures:
- int nc_http_open(const char* objecturl, void** curlp, fileoffset_t* filelenp);
- int nc_http_read(void* curl, const char* url, fileoffset_t start, fileoffset_t count, NCbytes* buf);
- int nc_http_close(void* curl);
- typedef long long fileoffset_t;

The type *fileoffset_t* is used to avoid use of *off_t* or *off64_t*
which are too volatile. It is intended to be represent file lengths
and offsets.

##### nc_http_open
The *nc_http_open* procedure creates a *Curl* handle and returns it
in the *curlp* argument. It also obtains and searches the headers
looking for two headers:

1. "Accept-Ranges: bytes" -- to verify that byte-range access is supported.
2. "Content-Length: ..." -- to obtain the size of the remote dataset.

The dataset length is returned in the *filelenp* argument.

#### nc_http_read

The *nc_http_read* procedure reads a specified set of contiguous bytes
as specified by the *start* and *count* arguments. It takes the *Curl*
handle produced by *nc_http_open* to indicate the server from which to read.

The *buf* argument is a pointer to an instance of type *NCbytes*, which
is a dynamically expandable byte vector (see the file *include/ncbytes.h*).

This procedure reads *count* bytes from the remote dataset starting at
the offset *start* position. The bytes are stored in *buf*.

#### nc_http_close

The *nc_http_close* function closes the *Curl* handle and does any
necessary cleanup.

### Cloud Access

The HDF5 library code-base also provides a Virtual File Drive (VFD)
capable of providing byte-range access to cloud storage
(Amazon S3 specifically).

This VFD is called *H5FDros3*. In order for the netcdf library
to make use of it, the HDF5 library must be built using the
*--enable-ros3-vfd* option.
Netcdf can discover that this capability was enabled and can
then make use of it to provide byte-range access to the cloud.

# Point of Contact {#byterange_poc}

__Author__: Dennis Heimbigner<br>
__Email__: dmh at ucar dot edu<br>
__Initial Version__: 12/30/2018<br>
__Last Revised__: 3/11/2023

<!-- End MarkDown -->
