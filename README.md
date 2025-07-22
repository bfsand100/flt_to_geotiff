# flt_to_geotiff
c code that converts between .flt and .geotiff formatted raster data files

#Basic Information

This package includes both flt_to_geotiff and geotiff_to_flt codes for conversion 
between .flt (raw list of nrow*ncol 32 bit floating point numbers) and .tif data
files representing a single variable across a grid that is nrows by ncols in size.

The package utilizes gdal libraries, which need to be available at the time of
compilation. 

It is assumed that the .flt data file stores data in column-major order, starting
in the upper left hand corner. This means that in reading data, the code will 
assume that the very first 32 bits of data represent the number in the upper left
corner, the second 32 bits represents the number underneath it, and after reading a
total of nrows.

The .flt file must be paired with an ascii *.hdr file that contains information similar
this:

ncols        3000
nrows        5000
xllcorner   396994
yllcorner   3779994
cellsize   2
nodata_value       -9999

The code does not retain information about the coordinate system or projection.

#Compilation

A makefile is included to automate complilation, although the user will likeley need to 
update the flags for the compiler and the gdal library. 

Assuming the gdal library is available in the operating directory, compilation could
be as simple as

gcc -o flt_to_geotiff flt_to_geotff.c

gcc -o geotiff_to_flt getiff_to_flt

#Usage

To create an .tif, there are two input files and one output file

./flt_to_geotiff <fname>.flt <fname>.hdr <fname>.tif

To create an .flt, there is one input files and two output files

./geotiff_to_flt <fname>.tif <fname>.flt <fname>.hdr

#Tips

Large-language AI models are really good at helping with installation and usage.


