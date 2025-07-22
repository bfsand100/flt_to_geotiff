#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "gdal.h"
#include "cpl_conv.h" // for CPLMalloc()

void write_hdr_file(const char *hdr_filename, int ncols, int nrows,
                    double xllcorner, double yllcorner, double cellsize, float nodata) {
    FILE *hdr = fopen(hdr_filename, "w");
    if (!hdr) {
        perror("Error creating .hdr file");
        return;
    }

    fprintf(hdr, "ncols         %d\n", ncols);
    fprintf(hdr, "nrows         %d\n", nrows);
    fprintf(hdr, "xllcorner     %.10f\n", xllcorner);
    fprintf(hdr, "yllcorner     %.10f\n", yllcorner);
    fprintf(hdr, "cellsize      %.10f\n", cellsize);
    fprintf(hdr, "NODATA_value  %.10f\n", nodata);

    fclose(hdr);
}

int main(int argc, char *argv[]) {
    if (argc != 4) {
        fprintf(stderr, "Usage: %s input.tif output.flt output.hdr\n", argv[0]);
        return 1;
    }

    const char *tif_filename = argv[1];
    const char *flt_filename = argv[2];
    const char *hdr_filename = argv[3];

    GDALAllRegister();
    GDALDatasetH dataset = GDALOpen(tif_filename, GA_ReadOnly);
    if (!dataset) {
        fprintf(stderr, "Failed to open GeoTIFF: %s\n", tif_filename);
        return 1;
    }

    int ncols = GDALGetRasterXSize(dataset);
    int nrows = GDALGetRasterYSize(dataset);
    GDALRasterBandH band = GDALGetRasterBand(dataset, 1);
    float nodata = (float)GDALGetRasterNoDataValue(band, NULL);

    float *row_buf = (float *)CPLMalloc(sizeof(float) * ncols);
    float *column_major = (float *)malloc(sizeof(float) * ncols * nrows);
    if (!column_major) {
        fprintf(stderr, "Memory allocation failed.\n");
        GDALClose(dataset);
        CPLFree(row_buf);
        return 1;
    }

    // Read GeoTIFF row-by-row and fill column-major array
    for (int row = 0; row < nrows; ++row) {
        if (GDALRasterIO(band, GF_Read, 0, row, ncols, 1, row_buf, ncols, 1, GDT_Float32, 0, 0) != CE_None) {
            fprintf(stderr, "Error reading row %d\n", row);
            free(column_major);
            CPLFree(row_buf);
            GDALClose(dataset);
            return 1;
        }

        for (int col = 0; col < ncols; ++col) {
            int dst_idx = col * nrows + row; // column-major order
            column_major[dst_idx] = row_buf[col];
        }
    }

    CPLFree(row_buf);

    // Write .flt file
    FILE *flt_fp = fopen(flt_filename, "wb");
    if (!flt_fp) {
        perror("Failed to create .flt file");
        free(column_major);
        GDALClose(dataset);
        return 1;
    }

    if (fwrite(column_major, sizeof(float), nrows * ncols, flt_fp) != (size_t)(nrows * ncols)) {
        fprintf(stderr, "Error writing to .flt file.\n");
    }

    fclose(flt_fp);
    free(column_major);

    // Get geotransform info
    double geo[6];
    if (GDALGetGeoTransform(dataset, geo) != CE_None) {
        fprintf(stderr, "Failed to read geotransform; using default values.\n");
        geo[0] = 0.0;          // xllcorner
        geo[1] = 1.0;          // cellsize (x)
        geo[2] = 0.0;
        geo[3] = 0.0;          // y origin
        geo[4] = 0.0;
        geo[5] = -1.0;         // cellsize (y, negative for north-up)
    }

    // Compute lower-left corner from top-left origin
    double xllcorner = geo[0];
    double yllcorner = geo[3] + geo[5] * nrows;

    write_hdr_file(hdr_filename, ncols, nrows, xllcorner, yllcorner, geo[1], nodata);

    GDALClose(dataset);
    printf("Created: %s and %s\n", flt_filename, hdr_filename);
    return 0;
}
