#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "gdal.h"
#include "cpl_conv.h" // for CPLMalloc()

typedef struct {
    double xllcorner;
    double yllcorner;
    double cellsize;
    double NODATA_value;
    int nrows;
    int ncols;
} RasterHeader;

int parse_header(const char *hdr_filename, RasterHeader *header) {
    FILE *fp = fopen(hdr_filename, "r");
    if (!fp) {
        perror("Failed to open header file");
        return 0;
    }

    char key[64];
    while (fscanf(fp, "%s", key) != EOF) {
        if (strcmp(key, "ncols") == 0) fscanf(fp, "%d", &header->ncols);
        else if (strcmp(key, "nrows") == 0) fscanf(fp, "%d", &header->nrows);
        else if (strcmp(key, "xllcorner") == 0) fscanf(fp, "%lf", &header->xllcorner);
        else if (strcmp(key, "yllcorner") == 0) fscanf(fp, "%lf", &header->yllcorner);
        else if (strcmp(key, "cellsize") == 0) fscanf(fp, "%lf", &header->cellsize);
        else if (strcmp(key, "NODATA_value") == 0) fscanf(fp, "%lf", &header->NODATA_value);
    }

    fclose(fp);
    return 1;
}

int main(int argc, char *argv[]) {
    if (argc != 4) {
        fprintf(stderr, "Usage: %s input.flt input.hdr output.tif\n", argv[0]);
        return 1;
    }

    const char *flt_filename = argv[1];
    const char *hdr_filename = argv[2];
    const char *tif_filename = argv[3];

    RasterHeader header;
    if (!parse_header(hdr_filename, &header)) {
        fprintf(stderr, "Failed to parse header file.\n");
        return 1;
    }

    // Allocate memory for raster
    size_t num_pixels = (size_t)header.nrows * header.ncols;
    float *raster_data = (float *)malloc(num_pixels * sizeof(float));
    if (!raster_data) {
        fprintf(stderr, "Memory allocation failed.\n");
        return 1;
    }

    // Read binary data
    FILE *flt_fp = fopen(flt_filename, "rb");
    if (!flt_fp) {
        perror("Failed to open .flt file");
        free(raster_data);
        return 1;
    }

    size_t read_items = fread(raster_data, sizeof(float), num_pixels, flt_fp);
    fclose(flt_fp);
    if (read_items != num_pixels) {
        fprintf(stderr, "Error reading .flt file. Expected %zu, got %zu floats.\n", num_pixels, read_items);
        free(raster_data);
        return 1;
    }

    // Initialize GDAL
    GDALAllRegister();

    GDALDriverH driver = GDALGetDriverByName("GTiff");
    if (!driver) {
        fprintf(stderr, "GTiff driver not available.\n");
        free(raster_data);
        return 1;
    }

    GDALDatasetH dataset = GDALCreate(driver, tif_filename, header.ncols, header.nrows, 1, GDT_Float32, NULL);
    if (!dataset) {
        fprintf(stderr, "Failed to create output TIFF file.\n");
        free(raster_data);
        return 1;
    }

    // Set geotransform (top-left x, w-e pixel resolution, 0, top-left y, 0, n-s pixel resolution)
    double geotransform[6] = {
        header.xllcorner,             // top left x
        header.cellsize,              // w-e pixel resolution
        0,                            // rotation (0)
        header.yllcorner + header.cellsize * header.nrows,  // top left y
        0,                            // rotation (0)
        -header.cellsize              // n-s pixel resolution (negative because Y decreases)
    };
    GDALSetGeoTransform(dataset, geotransform);

    // Write data (row order from top to bottom)
    GDALRasterBandH band = GDALGetRasterBand(dataset, 1);
    GDALSetRasterNoDataValue(band, header.NODATA_value);

    for (int row = 0; row < header.nrows; ++row) {
        int reversed_row = header.nrows - 1 - row;  // Flip vertical order
        float *row_data = raster_data + reversed_row * header.ncols;
        GDALRasterIO(band, GF_Write, 0, row, header.ncols, 1, row_data, header.ncols, 1, GDT_Float32, 0, 0);
    }

    // Cleanup
    GDALClose(dataset);
    free(raster_data);

    printf("GeoTIFF written to: %s\n", tif_filename);
    return 0;
}
