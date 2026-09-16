/**
 * @file thermocouple.c
 * @author anderson@eemn.io
 * @brief
 *
 * @copyright Copyright (c) 2025 Embedded Design Solutions, LLC.  All Rights Reserved.
 *
 */

/*******************************************************************************
 * Includes
 *******************************************************************************/
#include "thermocouple.h"

#include <math.h>
#include <stdint.h>

/*******************************************************************************
 * Module Macros
 *******************************************************************************/

/*******************************************************************************
 * Module Typedefs
 *******************************************************************************/
// table data and equation comes from:
//     https://its90.nist.gov/InvFunctions
//     https://its90.nist.gov/RefFunctions
typedef struct {
    double Ei;
    double Ef;
    double Ti;
    double Tf;
    int ord;
    double d[15];
} coef_row_t;

typedef struct {
    thermo_type_enum type;
    coef_row_t rows[3];
    double Ei;
    double Ef;
    double Ti;
    double Tf;
} coef_table_t;

/*******************************************************************************
 * Module Variable Definitions
 *******************************************************************************/

static coef_table_t e_table = {.type = THERMO_TYPE_E,
                               .Ti = -270.0f,
                               .Tf = 1000,
                               .rows = {
                                   {.Ti = -270.0,
                                    .Tf = 0.0,
                                    .ord = 13,
                                    .d = {0.00E+00, 5.87E-02, 4.54E-05, -7.80E-07, -2.58E-08, -5.95E-10, -9.32E-12,
                                          -1.03E-13, -8.04E-16, -4.40E-18, -1.64E-20, -3.97E-23, -5.58E-26, -3.47E-29}},
                                   {.Ti = 0.0,
                                    .Tf = 1000.0,
                                    .ord = 10,
                                    .d = {0.00E+00, 5.87E-02, 4.50E-05, 2.89E-08, -3.31E-10, 6.50E-13, -1.92E-16,
                                          -1.25E-18, 2.15E-21, -1.44E-24, 3.60E-28}},
                               }};

static coef_table_t j_table = {
    .type = THERMO_TYPE_J,
    .Ti = -210.0,
    .Tf = 1200,
    .rows = {
        {.Ti = -210.0,
         .Tf = 760.0,
         .ord = 8,
         .d = {0.00E+00, 5.04E-02, 3.05E-05, -8.57E-08, 1.32E-10, -1.71E-13, 2.09E-16, -1.25E-19, 1.56E-23}},
        {.Ti = 760.0, .Tf = 1200.0, .ord = 5, .d = {2.96E+02, -1.50E+00, 3.18E-03, -3.18E-06, 1.57E-09, -3.07E-13}},
    }};

static coef_table_t k_table = {.type = THERMO_TYPE_K,
                               .Ti = -270.0f,
                               .Tf = 1372,
                               .rows = {
                                   {.Ti = -270,
                                    .Tf = 0,
                                    .ord = 10,
                                    .d = {0.00E+00, 3.95E-02, 2.36E-05, -3.29E-07, -4.99E-09, -6.75E-11, -5.74E-13,
                                          -3.11E-15, -1.05E-17, -1.99E-20, -1.63E-23}},
                                   {.Ti = 0.0,
                                    .Tf = 1372.0,
                                    .ord = 9,
                                    .d = {-1.76E-02, 3.89E-02, 1.86E-05, -9.95E-08, 3.18E-10, -5.61E-13, 5.61E-16,
                                          -3.20E-19, 9.72E-23, -1.21E-26}},
                               }};

static coef_table_t inverse_e_table = {
    .type = THERMO_TYPE_E,
    .Ei = -8.825f,
    .Ef = 76.373f,
    .rows = {
        {.Ei = -8.825f,
         .Ef = 0.0f,
         .ord = 9,
         .d =
             {
                 0.0000000E+00,
                 1.6977288E+01,
                 -4.3514970E-01,
                 -1.5859697E-01,
                 -9.2502871E-02,
                 -2.6084314E-02,
                 -4.1360199E-03,
                 -3.4034030E-04,
                 -1.1564890E-05,
                 0.0000000E+00,
             }},
        {.Ei = 0.0f,
         .Ef = 76.373f,
         .ord = 9,
         .d = {0.0000000E+00, 1.7057035E+01, -2.3301759E-01, 6.5435585E-03, -7.3562749E-05, -1.7896001E-06,
               8.4036165E-08, -1.3735879E-09, 1.0629823E-11, -3.2447087E-14}},
    }};

static coef_table_t inverse_j_table = {.type = THERMO_TYPE_J,
                                       .Ei = -8.095f,
                                       .Ef = 69.553f,
                                       .rows = {
                                           {.Ei = -8.095f,
                                            .Ef = 0.0f,
                                            .ord = 8,
                                            .d =
                                                {
                                                    0.0000000E+00,
                                                    1.9528268E+01,
                                                    -1.2286185E+00,
                                                    -1.0752178E+00,
                                                    -5.9086933E-01,
                                                    -1.7256713E-01,
                                                    -2.8131513E-02,
                                                    -2.3963370E-03,
                                                    -8.3823321E-05,
                                                }},
                                           {.Ei = 0.0f,
                                            .Ef = 42.919f,
                                            .ord = 8,
                                            .d =
                                                {
                                                    0.000000E+00,
                                                    1.978425E+01,
                                                    -2.001204E-01,
                                                    1.036969E-02,
                                                    -2.549687E-04,
                                                    3.585153E-06,
                                                    -5.344285E-08,
                                                    5.099890E-10,
                                                    0.000000E+00,
                                                }},
                                           {.Ei = 42.919f,
                                            .Ef = 69.553f,
                                            .ord = 8,
                                            .d =
                                                {
                                                    -3.11358187E+03,
                                                    3.00543684E+02,
                                                    -9.94773230E+00,
                                                    1.70276630E-01,
                                                    -1.43033468E-03,
                                                    4.73886084E-06,
                                                    0.00000000E+00,
                                                    0.00000000E+00,
                                                    0.00000000E+00,
                                                }},
                                       }};

static coef_table_t inverse_k_table = {.type = THERMO_TYPE_K,
                                       .Ei = -5.891f,
                                       .Ef = 54.886f,
                                       .rows = {
                                           {.Ei = -5.891f,
                                            .Ef = 0.0f,
                                            .ord = 9,
                                            .d =
                                                {
                                                    0.0000000E+00,
                                                    2.5173462E+01,
                                                    -1.1662878E+00,
                                                    -1.0833638E+00,
                                                    -8.9773540E-01,
                                                    -3.7342377E-01,
                                                    -8.6632643E-02,
                                                    -1.0450598E-02,
                                                    -5.1920577E-04,
                                                    0.0000000E+00,
                                                }},
                                           {.Ei = 0.0f,
                                            .Ef = 20.644f,
                                            .ord = 9,
                                            .d =
                                                {
                                                    0.000000E+00,
                                                    2.508355E+01,
                                                    7.860106E-02,
                                                    -2.503131E-01,
                                                    8.315270E-02,
                                                    -1.228034E-02,
                                                    9.804036E-04,
                                                    -4.413030E-05,
                                                    1.057734E-06,
                                                    -1.052755E-08,
                                                }},
                                           {.Ei = 20.644f,
                                            .Ef = 54.886f,
                                            .ord = 9,
                                            .d =
                                                {
                                                    -1.318058E+02,
                                                    4.830222E+01,
                                                    -1.646031E+00,
                                                    5.464731E-02,
                                                    -9.650715E-04,
                                                    8.802193E-06,
                                                    -3.110810E-08,
                                                    0.000000E+00,
                                                    0.000000E+00,
                                                    0.000000E+00,
                                                }},
                                       }};

static coef_table_t* thermo_tables[] = {
    &j_table,
    &e_table,
    &k_table,
};

static coef_table_t* thermo_inverse_tables[] = {
    &inverse_j_table,
    &inverse_e_table,
    &inverse_k_table,
};

/*******************************************************************************
 * Function Prototypes
 *******************************************************************************/
static coef_table_t* get_table(thermo_type_enum type);
static coef_table_t* get_inverse_table(thermo_type_enum type);
static coef_row_t* get_row_by_emf(coef_table_t* table, double E);
static coef_row_t* get_row_by_t(coef_table_t* table, double T90);
static double powerofXY(double X, double Y);
static double inverse_polynomial(coef_row_t* coefs, double emf);
static double polynomial(coef_row_t* coefs, double T90);
static double k_term(double t90);

/*******************************************************************************
 * Public Function Definitions
 *******************************************************************************/
double thermocouple_get_emf(thermo_type_enum type, double T90)
{
    coef_table_t* table = get_table(type);
    if(0 == table) {
        return 0;
    }
    if(T90 < table->Ti || T90 > table->Tf) {
        return 0;
    }
    coef_row_t* row = get_row_by_t(table, T90);
    double emf = polynomial(row, T90);
    if(THERMO_TYPE_K == type) {
        emf += k_term(T90);
    }
    return emf;
}

double thermocouple_get_temp(thermo_type_enum type, double emf)
{
    coef_table_t* table = get_inverse_table(type);
    if(0 == table) {
        return 0;
    }
    if(emf < table->Ei || emf > table->Ef) {
        return 0;
    }
    coef_row_t* row = get_row_by_emf(table, emf);
    return inverse_polynomial(row, emf);
}

/*******************************************************************************
 * Private Function Definitions
 *******************************************************************************/
static double powerofXY(double X, double Y)
{
    double num;
    int i;
    num = X;  // copy the value to a variable
    if((uint32_t)Y == 0) {
        X = 1;  // if X^0 = 1
    } else if((uint32_t)Y == 1) {
        X = num;  // X^1 = X
    } else {
        for(i = 2; i <= Y; i++) {  // X^Y where Y>1
            X = X * num;
        }
    }
    return X;
}

static coef_table_t* get_table(thermo_type_enum type)
{
    for(int i = 0; i < (int)( sizeof(thermo_tables) / sizeof(coef_table_t*) ); i++) {
        if(type == thermo_tables[i]->type) {
            return thermo_tables[i];
        }
    }
    return 0;
}

static coef_table_t* get_inverse_table(thermo_type_enum type)
{
    for(int i = 0; i < (int)(sizeof(thermo_inverse_tables) / sizeof(coef_table_t*)); i++) {
        if(type == thermo_inverse_tables[i]->type) {
            return thermo_inverse_tables[i];
        }
    }
    return 0;
}

// selects the row where E  is within the bounds (Ei,Ef)
static coef_row_t* get_row_by_t(coef_table_t* table, double T90)
{
    int i = 0;
    for(i = 0; i < 3; i++) {
        if(table->rows[i].Ti <= T90 && T90 <= table->rows[i].Tf) {
            return &(table->rows[i]);
        }
    }
    return (coef_row_t*)0;
}

// selects the row where E  is within the bounds (Ei,Ef)
static coef_row_t* get_row_by_emf(coef_table_t* table, double E)
{
    int i = 0;
    for(i = 0; i < 3; i++) {
        if(table->rows[i].Ei <= E && E <= table->rows[i].Ef) {
            return &(table->rows[i]);
        }
    }
    return (coef_row_t*)0;
}

static double inverse_polynomial(coef_row_t* coefs, double emf)
{
    double T90 = 0;
    for(int i = 0; i < (coefs->ord + 1); i++) {
        double add = coefs->d[i] * powerofXY(emf, i);
        T90 += add;
    }
    return T90;
}

static double polynomial(coef_row_t* coefs, double T90)
{
    double E = 0;
    for(int i = 0; i < (coefs->ord + 1); i++) {
        double add = coefs->d[i] * powerofXY(T90, i);
        E += add;
    }
    return E;
}

static double k_term(double t90)
{
    const double a0 = 0.118597600000e+0;
    const double a1 = -0.118343200000e-3;
    const double a2 = 0.126968600000e3;
    double exponent = a1 * (t90 - a2) * (t90 - a2);
    return a0 * exp(exponent);
}
