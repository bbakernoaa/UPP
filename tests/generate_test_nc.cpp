#include <iostream>
#include <vector>
#include <netcdf.h>

#define NC_CHECK(err) \
    if ((err) != NC_NOERR) { \
        std::cerr << "NetCDF Error at line " << __LINE__ << ": " << nc_strerror(err) << std::endl; \
        return 1; \
    }

int main() {
    std::cout << "Generating test meteorological data in input.nc..." << std::endl;
    int ncid, x_dim, y_dim, z_dim;
    int t_var, q_var, p_var, sfc_var;

    // Define shapes dynamically: asymmetric 4 x 3 grid, 5 vertical levels
    int dims_3d[3];
    int dims_2d[2];

    NC_CHECK(nc_create("input.nc", NC_CLOBBER, &ncid));

    NC_CHECK(nc_def_dim(ncid, "x", 4, &x_dim));
    NC_CHECK(nc_def_dim(ncid, "y", 3, &y_dim));
    NC_CHECK(nc_def_dim(ncid, "z", 5, &z_dim));

    dims_3d[0] = x_dim; dims_3d[1] = y_dim; dims_3d[2] = z_dim;
    dims_2d[0] = x_dim; dims_2d[1] = y_dim;

    NC_CHECK(nc_def_var(ncid, "t", NC_DOUBLE, 3, dims_3d, &t_var));
    NC_CHECK(nc_def_var(ncid, "q", NC_DOUBLE, 3, dims_3d, &q_var));
    NC_CHECK(nc_def_var(ncid, "p", NC_DOUBLE, 3, dims_3d, &p_var));
    NC_CHECK(nc_def_var(ncid, "sfc_g", NC_DOUBLE, 2, dims_2d, &sfc_var));

    NC_CHECK(nc_enddef(ncid));

    // Populate data
    std::vector<double> t_data(4 * 3 * 5, 280.0);
    std::vector<double> q_data(4 * 3 * 5, 0.005);
    std::vector<double> p_data = {
        100000.0, 100000.0, 100000.0, 100000.0,
        80000.0,  80000.0,  80000.0,  80000.0,
        60000.0,  60000.0,  60000.0,  60000.0,
        40000.0,  40000.0,  40000.0,  40000.0,
        10000.0,  10000.0,  10000.0,  10000.0
    };
    std::vector<double> p_data_full;
    for (int k = 0; k < 12; ++k) {
        for (int l = 0; l < 5; ++l) {
            p_data_full.push_back(p_data[l]);
        }
    }
    std::vector<double> sfc_data(12, 0.0);

    NC_CHECK(nc_put_var_double(ncid, t_var, t_data.data()));
    NC_CHECK(nc_put_var_double(ncid, q_var, q_data.data()));
    NC_CHECK(nc_put_var_double(ncid, p_var, p_data_full.data()));
    NC_CHECK(nc_put_var_double(ncid, sfc_var, sfc_data.data()));

    NC_CHECK(nc_close(ncid));
    std::cout << "input.nc successfully generated." << std::endl;
    return 0;
}
