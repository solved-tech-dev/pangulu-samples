#ifndef SAMPLE_COMMON_H
#define SAMPLE_COMMON_H
#define _POSIX_C_SOURCE 200809L
/*
 * Shared file-reading, upload, timing, and residual helpers for the samples.
 *
 * Everything here reads MatrixMarket (.mtx) matrices and plain-text right-hand
 * side (.vec) files, builds the caller-visible CSC/CSR structures, and checks
 * the relative residual.  The main sample selects a precision, a
 * layout, a memory space, a reordering level and a phase sequence.
 *
 * A .vec file is plain text:
 *   - real    : n values, one per line
 *   - complex : n lines, each "re im"
 * The vector length is the matrix order, so no header is stored.
 */
#include "pangulu.h"

#include <cuda_runtime.h>
#include <math.h>
#include <stdint.h>
#include <float.h>
#include <limits.h>
#include <strings.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

typedef struct
{
    int64_t m, n, nnz;
    int64_t *ptr;    /* CSC colptr or CSR rowptr, length n+1 */
    int32_t *idx;    /* CSC rowidx or CSR colidx, length nnz */
    double *values;  /* nnz doubles, or 2*nnz interleaved re/im for complex */
    int cplx;        /* 0 = real, 1 = complex */
    int is_csr;      /* 0 = CSC, 1 = CSR */
} spmat_t;

static double wall_seconds(void)
{
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (double)ts.tv_sec + 1.0e-9 * (double)ts.tv_nsec;
}

static void spmat_free(spmat_t *a)
{
    free(a->ptr);
    free(a->idx);
    free(a->values);
    memset(a, 0, sizeof(*a));
}

/* Legacy customer fixtures sometimes omit the real field and one leading
 * percent sign. Accept that exact four-token real header, never guess complex. */
static int read_mtx_header(const char *line,char field[32],char sym[32])
{
    char banner[64],object[32],layout[32];
    int tokens=sscanf(line,"%63s %31s %31s %31s %31s",banner,object,layout,field,sym);
    if(tokens<4)return -1;
    if(tokens==4 && !strcasecmp(banner,"%MatrixMarket") &&
       (!strcasecmp(field,"general") || !strcasecmp(field,"symmetric"))){
        strcpy(sym,field);strcpy(field,"real");tokens=5;
    }else if(strcasecmp(banner,"%%MatrixMarket"))return -1;
    return tokens==5 && !strcasecmp(object,"matrix") && !strcasecmp(layout,"coordinate") ? 0:-1;
}

/* Read a full coordinate matrix. Symmetric/Hermitian/skew input is expanded
 * before either the solver or the independent residual calculation sees it. */
static int read_mtx_coordinate(const char *path, int want_csr,
                               int require_complex, spmat_t *a)
{
    FILE *f=fopen(path,"r"); char line[4096],field[32],sym[32];
    long long m=0,n=0,reported=0; int status=-1,is_complex=0,mirror=0,conjugate=0,negate=0,pattern=0;
    int64_t *coo_row=NULL,*coo_col=NULL,*counts=NULL,*cursor=NULL; double *coo_value=NULL;
    memset(a,0,sizeof(*a)); if(!f)return -1;
    if(!fgets(line,sizeof(line),f)||read_mtx_header(line,field,sym)!=0)goto done;
    is_complex=!strcasecmp(field,"complex"); pattern=!strcasecmp(field,"pattern");
    if(is_complex!=require_complex || (!is_complex&&!pattern&&strcasecmp(field,"real")&&strcasecmp(field,"integer")))goto done;
    if(!strcasecmp(sym,"symmetric"))mirror=1;
    else if(!strcasecmp(sym,"hermitian")){if(!is_complex)goto done;mirror=1;conjugate=1;}
    else if(!strcasecmp(sym,"skew-symmetric")){mirror=1;negate=1;}
    else if(strcasecmp(sym,"general"))goto done;
    do {if(!fgets(line,sizeof(line),f))goto done;}while(line[0]=='%');
    if(sscanf(line,"%lld %lld %lld",&m,&n,&reported)!=3 || m!=n || n<=0 || n>INT32_MAX || reported<0 ||
       (uint64_t)reported>SIZE_MAX/(4*sizeof(double)) || reported>INT64_MAX/2)goto done;
    size_t stored=(size_t)(reported?reported:1),components=is_complex?2u:1u;
    coo_row=malloc(stored*sizeof(*coo_row));coo_col=malloc(stored*sizeof(*coo_col));coo_value=malloc(stored*components*sizeof(*coo_value));
    counts=calloc((size_t)n+1,sizeof(*counts));cursor=malloc((size_t)n*sizeof(*cursor));
    if(!coo_row||!coo_col||!coo_value||!counts||!cursor)goto done;
    for(int64_t k=0;k<reported;++k){
        long long row=0,col=0;double re=1,im=0;
        do {if(!fgets(line,sizeof(line),f))goto done;}while(line[0]=='%' || line[0]=='\n' || line[0]=='\r');
        int got=is_complex?sscanf(line,"%lld %lld %lf %lf",&row,&col,&re,&im):pattern?sscanf(line,"%lld %lld",&row,&col):sscanf(line,"%lld %lld %lf",&row,&col,&re);
        if(got!=(is_complex?4:pattern?2:3)||row<1||row>n||col<1||col>n||!isfinite(re)||!isfinite(im))goto done;
        if(row==col && ((conjugate&&im!=0) || (negate&&(re!=0||im!=0))))goto done;
        coo_row[k]=--row;coo_col[k]=--col;coo_value[components*k]=re;if(is_complex)coo_value[2*k+1]=im;
        ++counts[(want_csr?row:col)+1];if(mirror&&row!=col)++counts[(want_csr?col:row)+1];
    }
    for(int64_t j=0;j<n;++j)counts[j+1]+=counts[j];
    size_t full=(size_t)(counts[n]?counts[n]:1);
    if(full>SIZE_MAX/(components*sizeof(double)))goto done;
    a->ptr=malloc(((size_t)n+1)*sizeof(*a->ptr));a->idx=malloc(full*sizeof(*a->idx));a->values=malloc(full*components*sizeof(*a->values));
    if(!a->ptr||!a->idx||!a->values)goto done;
    memcpy(a->ptr,counts,((size_t)n+1)*sizeof(*counts));memcpy(cursor,counts,(size_t)n*sizeof(*cursor));
    for(int64_t k=0;k<reported;++k){
        int64_t row=coo_row[k],col=coo_col[k],at=cursor[want_csr?row:col]++;
        double re=coo_value[components*k],im=is_complex?coo_value[2*k+1]:0;
        a->idx[at]=(int32_t)(want_csr?col:row);a->values[components*at]=re;if(is_complex)a->values[2*at+1]=im;
        if(mirror&&row!=col){at=cursor[want_csr?col:row]++;a->idx[at]=(int32_t)(want_csr?row:col);
            a->values[components*at]=negate?-re:re;if(is_complex)a->values[2*at+1]=negate||conjugate?-im:im;}
    }
    a->m=m;a->n=n;a->nnz=counts[n];a->cplx=is_complex;a->is_csr=want_csr;status=0;
done:
    fclose(f);free(coo_row);free(coo_col);free(coo_value);free(counts);free(cursor);
    if (status)
        spmat_free(a);
    return status;
}
static int read_mtx_real(const char *path,int want_csr,spmat_t *a)
{ return read_mtx_coordinate(path,want_csr,0,a); }
static int read_mtx_complex(const char *path,spmat_t *a)
{ return read_mtx_coordinate(path,0,1,a); }

static int read_vec_real(const char *path, int64_t n, double *out)
{
    FILE *f = fopen(path, "r");
    if (f == NULL)
    {
        return -1;
    }
    for (int64_t i = 0; i < n; ++i)
    {
        if (fscanf(f, "%lf", &out[i]) != 1)
        {
            fclose(f);
            return -1;
        }
    }
    fclose(f);
    return 0;
}

/* Read n complex values ("re im" per line) into 2*n interleaved doubles. */
static int read_vec_complex(const char *path, int64_t n, double *out)
{
    FILE *f = fopen(path, "r");
    if (f == NULL)
    {
        return -1;
    }
    for (int64_t i = 0; i < n; ++i)
    {
        if (fscanf(f, "%lf %lf", &out[2 * i], &out[2 * i + 1]) != 2)
        {
            fclose(f);
            return -1;
        }
    }
    fclose(f);
    return 0;
}

static float *to_float32(const double *src, int64_t count)
{
    float *dst = (float *)malloc(sizeof(float) * (size_t)count);
    if (dst == NULL)
    {
        return NULL;
    }
    for (int64_t i = 0; i < count; ++i)
    {
        dst[i] = (float)src[i];
    }
    return dst;
}

/* Upload the three matrix arrays (ptr, idx, values) to the active device.
 * values_bytes is the total byte size of the typed values buffer. */
static int upload_matrix_device(const int64_t *ptr, const int32_t *idx,
                                const void *values, int64_t n, int64_t nnz,
                                size_t values_bytes, int64_t **d_ptr,
                                int32_t **d_idx, void **d_values)
{
    const size_t ptr_bytes = sizeof(int64_t) * (size_t)(n + 1);
    const size_t idx_bytes = sizeof(int32_t) * (size_t)nnz;
    if (cudaMalloc((void **)d_ptr, ptr_bytes) != cudaSuccess ||
        cudaMalloc((void **)d_idx, idx_bytes) != cudaSuccess ||
        cudaMalloc(d_values, values_bytes) != cudaSuccess ||
        cudaMemcpy(*d_ptr, ptr, ptr_bytes, cudaMemcpyHostToDevice) !=
            cudaSuccess ||
        cudaMemcpy(*d_idx, idx, idx_bytes, cudaMemcpyHostToDevice) !=
            cudaSuccess ||
        cudaMemcpy(*d_values, values, values_bytes, cudaMemcpyHostToDevice) !=
            cudaSuccess)
    {
        return -1;
    }
    return 0;
}

/* Run ANALYSIS -> NUMERIC_FACTORISATION -> SOLVE, timing each phase. */
static int run_analysis_numeric_solve(pangulu_handle_t h, pangulu_matrix_t A,
                                      pangulu_vector_t X, pangulu_vector_t B)
{
    const pangulu_phase_t phases[3] = {
        PANGULU_PHASE_ANALYSIS, PANGULU_PHASE_NUMERIC_FACTORISATION,
        PANGULU_PHASE_SOLVE };
    const char *names[3] = { "Analysis", "Numeric factorisation", "Solving" };
    for (int i = 0; i < 3; ++i)
    {
        const double t0 = wall_seconds();
        const pangulu_status_t s = pangulu_execute(
            h, phases[i], A, i == 2 ? X : NULL, i == 2 ? B : NULL);
        const double dt = wall_seconds() - t0;
        if (s == PANGULU_STATUS_SUCCESS)
        {
            printf("%s time: %.3f ms\n", names[i], dt * 1.0e3);
        }
        if (s != PANGULU_STATUS_SUCCESS)
        {
            fprintf(stderr, "PanguLU phase '%s' failed with status %d\n",
                    names[i], (int)s);
            return -1;
        }
    }
    return 0;
}

/* Run one SOLVE phase (the factorization must already exist). */
static int run_solve(pangulu_handle_t h, pangulu_matrix_t A, pangulu_vector_t X,
                     pangulu_vector_t B)
{
    const double t0 = wall_seconds();
    const pangulu_status_t s = pangulu_execute(h, PANGULU_PHASE_SOLVE, A, X, B);
    const double dt = wall_seconds() - t0;
    if (s == PANGULU_STATUS_SUCCESS)
    {
        printf("Solving time: %.3f ms\n", dt * 1.0e3);
    }
    if (s != PANGULU_STATUS_SUCCESS)
    {
        fprintf(stderr, "PanguLU phase 'solve' failed with status %d\n", (int)s);
        return -1;
    }
    return 0;
}

/* Compute ||A*x-b||_inf / ||b||_inf (complex magnitudes for complex data).
 * Accumulate in long double: double cancellation can otherwise turn a
 * nonzero residual into an apparent zero. NaN must never enter a max reduction. */
static int residual_norms(const spmat_t *a,const double *x,const double *b,
                          long double *numerator,long double *denominator)
{
    const int components=a->cplx?2:1;long double *ax=calloc((size_t)a->n*components,sizeof(*ax));
    if (!ax)
        return -1;
    *numerator = 0;
    *denominator = 0;
    for(int64_t i=0;i<a->n*components;++i)if(!isfinite(x[i])||!isfinite(b[i])){free(ax);return -2;}
    for(int64_t outer=0;outer<a->n;++outer)for(int64_t p=a->ptr[outer];p<a->ptr[outer+1];++p){
        int64_t row=a->is_csr?outer:a->idx[p],col=a->is_csr?a->idx[p]:outer;
        long double ar=a->values[components*p],ai=components==2?a->values[2*p+1]:0;
        if(!isfinite(ar)||!isfinite(ai)){free(ax);return -2;}
        long double xr=x[components*col],xi=components==2?x[2*col+1]:0;
        ax[components*row]+=ar*xr-ai*xi;if(components==2)ax[2*row+1]+=ar*xi+ai*xr;
    }
    for(int64_t i=0;i<a->n;++i){
        long double re=ax[components*i]-b[components*i],im=components==2?ax[2*i+1]-b[2*i+1]:0;
        long double r=hypotl(re,im),bn=hypotl(b[components*i],components==2?b[2*i+1]:0);
        if(!isfinite(r)||!isfinite(bn)){free(ax);return -2;}
        if (r > *numerator)
            *numerator = r;
        if (bn > *denominator)
            *denominator = bn;
    }
    free(ax);return 0;
}
/* Compatibility helpers for tests; the printed result below keeps the full
 * long-double range. An unrepresentable double result is explicitly invalid. */
static double residual_real(const spmat_t *a,const double *x,const double *b)
{
    long double n=0,d=0;if(residual_norms(a,x,b,&n,&d))return NAN;
    long double v=d>0?n/d:n;if(!isfinite(v)||v>DBL_MAX)return NAN;
    double result=(double)v;return v>0&&result==0?nextafter(0.,1.):result;
}
static double residual_complex(const spmat_t *a,const double *x,const double *b)
{ return residual_real(a,x,b); }
#ifndef SAMPLE_RESIDUAL_TOLERANCE
#define SAMPLE_RESIDUAL_TOLERANCE 1.0e-10L
#endif

/* Validate the original system independently of API execution status. */
static int report_residual(const spmat_t *a, const double *x, const double *b)
{
    long double numerator = 0, denominator = 0;
    const int status = residual_norms(a, x, b, &numerator, &denominator);
    if (status != 0)
    {
        printf("verification: FAIL (%s)\n",
               status == -1 ? "residual allocation failed" :
                              "invalid/non-finite residual");
        return -1;
    }
    const long double residual = denominator > 0 ? numerator / denominator : numerator;
    const long double tolerance = SAMPLE_RESIDUAL_TOLERANCE;
    const int passed = isfinite(residual) && residual <= tolerance;
    printf("RHS infinity norm = %.12Le\n", denominator);
    if (denominator == 0)
        printf("|| Ax - b ||_inf = %.12Le (zero RHS; relative residual undefined)\n",
               residual);
    else
        printf("|| Ax - b || / || b || = %.12Le\n", residual);
    printf("verification: %s (tolerance %.3Le)\n", passed ? "PASS" : "FAIL", tolerance);
    return passed ? 0 : -1;
}

#endif /* SAMPLE_COMMON_H */
