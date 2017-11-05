#include "include/amici_model.h"
#include <cstring>
#include <include/edata.h>
#include <include/rdata.h>
#include <include/tdata.h>
#include <include/udata.h>

namespace amici {


UserData Model::getUserData() const { return UserData(np, nk, nx); }

UserData *Model::getNewUserData() const { return new UserData(np, nk, nx); }

Model::~Model() {
    if (z2event)
        delete[] z2event;

    if (idlist)
        delete[] idlist;
}

/** Sensitivity of measurements y, total derivative
 * @param[in] it timepoint index @type int
 * @param[in] tdata pointer to temp data object @type TempData
 * @param[in,out] rdata pointer to return data object @type ReturnData
 */
void Model::fsy(const int it, const TempData *tdata, ReturnData *rdata) {
    // Compute sy = dydx * sx + dydp

    for (int ip = 0; ip < rdata->nplist; ++ip) {
        for (int iy = 0; iy < ny; ++iy)
            // copy dydp to sy
            rdata->sy[ip * rdata->nt * ny + iy * rdata->nt + it] =
                tdata->dydp[iy + ip * ny];

        realtype *sx_tmp = N_VGetArrayPointer(tdata->sx[ip]);

        // compute sy = 1.0*dydx*sx + 1.0*sy
        amici_dgemv(AMICI_BLAS_ColMajor, AMICI_BLAS_NoTrans, ny, nx, 1.0,
                    tdata->dydx, ny, sx_tmp, 1, 1.0,
                    &rdata->sy[it + ip * rdata->nt * ny], rdata->nt);
    }
}

/** Sensitivity of z at final timepoint (ignores sensitivity of timepoint),
 * total derivative
 * @param[in] ie event index @type int
 * @param[in] tdata pointer to temp data object @type TempData
 * @param[in,out] rdata pointer to return data object @type ReturnData
 */
void Model::fsz_tf(const int ie, const TempData *tdata, ReturnData *rdata) {
    // Compute sz = dzdx * sz + dzdp

    for (int ip = 0; ip < rdata->nplist; ++ip) {
        for (int iz = 0; iz < nz; ++iz)
            // copy dydp to sy
            rdata->sz[tdata->nroots[ie] + (iz + ip * nz) * rdata->nmaxevent] =
                0;
    }
}

/** Sensitivity of time-resolved measurement negative log-likelihood Jy, total
 * derivative
 * @param[in] it timepoint index @type int
 * @param[in] tdata pointer to temp data object @type TempData
 * @param[in,out] rdata pointer to return data object @type ReturnData
 */
void Model::fsJy(const int it, const TempData *tdata, ReturnData *rdata) {

    // Compute dJydx*sx for current 'it'
    // dJydx        rdata->nt x nJ x nx
    // sx           rdata->nt x nx x rdata->nplist
    std::vector<double> multResult(nJ * rdata->nplist, 0);
    std::vector<double> sxTmp(rdata->nplist * nx, 0);
    
    for (int ix = 0; ix < nx; ++ix) {
        for (int ip = 0; ip < rdata->nplist; ++ip)
            sxTmp[ix + ip * nx] = rdata->sx[it + (ix + ip * nx) * rdata->nt];
        for (int iJ = 0; iJ < nJ; ++iJ)
            dJydxTmp[iJ + ix * nJ] =
                tdata->dJydx[it + (iJ + ix * nJ) * rdata->nt];
    }

    // C := alpha*op(A)*op(B) + beta*C,
    amici_dgemm(AMICI_BLAS_ColMajor, AMICI_BLAS_NoTrans, AMICI_BLAS_NoTrans, nJ,
                rdata->nplist, nx, 1.0, dJydxTmp.data(), nJ, sxTmp.data(), nx, 0.0,
                multResult.data(), nJ);

    // multResult    nJ x rdata->nplist
    // dJydp         nJ x rdata->nplist
    // dJydxTmp      nJ x nx
    // sxTmp         nx x rdata->nplist

    // sJy += multResult + dJydp
    for (int iJ = 0; iJ < nJ; ++iJ) {
        if (iJ == 0)
            for (int ip = 0; ip < rdata->nplist; ++ip)
                rdata->sllh[ip] -= multResult[ip * nJ] + tdata->dJydp[ip * nJ];
        else
            for (int ip = 0; ip < rdata->nplist; ++ip)
                rdata->s2llh[(iJ - 1) + ip * (nJ - 1)] -=
                    multResult[iJ + ip * nJ] + tdata->dJydp[iJ + ip * nJ];
    }
}

/** Sensitivity of time-resolved measurement negative log-likelihood Jy w.r.t.
 * parameters
 * @param[in] it timepoint index @type int
 * @param[in,out] tdata pointer to temp data object @type TempData
 * @param[in] edata pointer to experimental data object @type ExpData
 * @param[in] rdata pointer to return data object @type ReturnData
 */
void Model::fdJydp(const int it, TempData *tdata, const ExpData *edata,
                  const ReturnData *rdata) {

    // dJydy         nytrue x nJ x ny
    // dydp          ny x rdata->nplist
    // dJydp         nJ x rdata->nplist
    memset(tdata->dJydp, 0, nJ * rdata->nplist * sizeof(double));
    for (int iyt = 0; iyt < nytrue; ++iyt) {
        if (amiIsNaN(edata->my[rdata->nt * iyt + it]))
            continue;

        // copy current (iyt) dJydy and dJydsigma slices
        // dJydyTmp     nJ x ny
        // dJydsigmaTmp nJ x ny
        for (int iJ = 0; iJ < nJ; ++iJ) {
            for (int iy = 0; iy < ny; ++iy) {
                dJydyTmp[iJ + iy * nJ] =
                    tdata->dJydy[iyt + (iJ + iy * nJ) * nytrue];
                dJydsigmaTmp[iJ + iy * nJ] =
                    tdata->dJydsigma[iyt + (iJ + iy * nJ) * nytrue];
            }
        }

        amici_dgemm(AMICI_BLAS_ColMajor, AMICI_BLAS_NoTrans, AMICI_BLAS_NoTrans,
                    nJ, rdata->nplist, ny, 1.0, dJydyTmp.data(), nJ, tdata->dydp, ny,
                    1.0, tdata->dJydp, nJ);

        amici_dgemm(AMICI_BLAS_ColMajor, AMICI_BLAS_NoTrans, AMICI_BLAS_NoTrans,
                    nJ, rdata->nplist, ny, 1.0, dJydsigmaTmp.data(), nJ,
                    tdata->dsigmaydp, ny, 1.0, tdata->dJydp, nJ);
    }
}

/** Sensitivity of time-resolved measurement negative log-likelihood Jy w.r.t.
 * state variables
 * @param[in] it timepoint index @type int
 * @param[in,out] tdata pointer to temp data object @type TempData
 * @param[in] edata pointer to experimental data object @type ExpData
 */
void Model::fdJydx(const int it, TempData *tdata, const ExpData *edata) {

    // dJydy         nytrue x nJ x ny
    // dydx          ny x nx
    // dJydx         rdata->nt x nJ x nx
    
    std::vector<double> multResult(nJ * nx, 0);
    for (int iyt = 0; iyt < nytrue; ++iyt) {
        if (amiIsNaN(edata->my[tdata->rdata->nt * iyt + it]))
            continue;

        // copy current (iyt) dJydy slice
        // dJydyTmp     nJ x ny
        for (int iJ = 0; iJ < nJ; ++iJ)
            for (int iy = 0; iy < ny; ++iy)
                dJydyTmp[iJ + iy * nJ] =
                    tdata->dJydy[iyt + (iJ + iy * nJ) * nytrue];

        amici_dgemm(AMICI_BLAS_ColMajor, AMICI_BLAS_NoTrans, AMICI_BLAS_NoTrans,
                    nJ, nx, ny, 1.0, dJydyTmp.data(), nJ, tdata->dydx, ny, 1.0,
                    multResult.data(), nJ);
    }
    for (int iJ = 0; iJ < nJ; ++iJ)
        for (int ix = 0; ix < nx; ++ix)
            tdata->dJydx[it + (iJ + ix * nJ) * tdata->rdata->nt] =
                multResult[iJ + ix * nJ];
}

/** Sensitivity of event-resolved measurement negative log-likelihood Jz, total
 * derivative
 * @param[in] ie event index @type int
 * @param[in,out] tdata pointer to temp data object @type TempData
 * @param[in] rdata pointer to return data object @type ReturnData
 */
void Model::fsJz(const int ie, TempData *tdata, const ReturnData *rdata) {
    // sJz           nJ x rdata->nplist
    // dJzdp         nJ x rdata->nplist
    // dJzdx         nmaxevent x nJ x nx
    // sx            rdata->nt x nx x rdata->nplist

    // Compute dJzdx*sx for current 'ie'
    // dJzdx        rdata->nt x nJ x nx
    // sx           rdata->nt x nx x rdata->nplist

    std::vector<double> multResult(nJ * rdata->nplist, 0);
    std::vector<double> sxTmp(rdata->nplist * nx, 0);
    realtype *sx_tmp;
    for (int ip = 0; ip < rdata->nplist; ++ip) {
        sx_tmp = NV_DATA_S(tdata->sx[ip]);
        if (!sx_tmp) {
            throw NullPointerException("sx_tmp");
        }
        for (int ix = 0; ix < nx; ++ix)
            sxTmp[ix + ip * nx] = sx_tmp[ix];
    }

    for (int ix = 0; ix < nx; ++ix)
        for (int iJ = 0; iJ < nJ; ++iJ)
            dJzdxTmp[iJ + ix * nJ] =
                tdata->dJzdx[tdata->nroots[ie] +
                             (iJ + ix * nJ) * rdata->nmaxevent];

    // C := alpha*op(A)*op(B) + beta*C,
    amici_dgemm(AMICI_BLAS_ColMajor, AMICI_BLAS_NoTrans, AMICI_BLAS_NoTrans, nJ,
                rdata->nplist, nx, 1.0, dJzdxTmp.data(), nJ, sxTmp.data(), nx, 1.0,
                multResult.data(), nJ);

    // sJy += multResult + dJydp
    for (int iJ = 0; iJ < nJ; ++iJ) {
        if (iJ == 0)
            for (int ip = 0; ip < rdata->nplist; ++ip)
                rdata->sllh[ip] -= multResult[ip * nJ] + tdata->dJzdp[ip * nJ];
        else
            for (int ip = 0; ip < rdata->nplist; ++ip)
                rdata->s2llh[(iJ - 1) + ip * (nJ - 1)] -=
                    multResult[iJ + ip * nJ] + tdata->dJzdp[iJ + ip * nJ];
    }
}

/** Sensitivity of event-resolved measurement negative log-likelihood Jz w.r.t.
 * parameters
 * @param[in] ie event index @type int
 * @param[in,out] tdata pointer to temp data object @type TempData
 * @param[in] edata pointer to experimental data object @type ExpData
 * @param[in] rdata pointer to return data object @type ReturnData
 */
void Model::fdJzdp(const int ie, TempData *tdata, const ExpData *edata,
                  const ReturnData *rdata) {
    // dJzdz         nztrue x nJ x nz
    // dJzdsigma     nztrue x nJ x nz
    // dzdp          nz x rdata->nplist
    // dJzdp         nJ x rdata->nplist

    memset(tdata->dJzdp, 0, nJ * rdata->nplist * sizeof(double));

    for (int izt = 0; izt < nztrue; ++izt) {
        if (amiIsNaN(edata->mz[tdata->nroots[ie] + izt * rdata->nmaxevent]))
            continue;

        // copy current (izt) dJzdz and dJzdsigma slices
        // dJzdzTmp     nJ x nz
        // dJzdsigmaTmp nJ x nz

        if (tdata->t < rdata->ts[rdata->nt - 1]) {
            for (int iJ = 0; iJ < nJ; ++iJ) {
                for (int iz = 0; iz < nz; ++iz) {
                    dJzdzTmp[iJ + iz * nJ] =
                        tdata->dJzdz[izt + (iJ + iz * nJ) * nztrue];
                }
            }
        } else {
            for (int iJ = 0; iJ < nJ; ++iJ) {
                for (int iz = 0; iz < nz; ++iz) {
                    dJzdzTmp[iJ + iz * nJ] =
                        tdata->dJrzdz[izt + (iJ + iz * nJ) * nztrue];
                    dJrzdsigmaTmp[iJ + iz * nJ] =
                        tdata->dJrzdsigma[izt + (iJ + iz * nJ) * nztrue];
                }
            }
            amici_dgemm(AMICI_BLAS_ColMajor, AMICI_BLAS_NoTrans,
                        AMICI_BLAS_NoTrans, nJ, rdata->nplist, nz, 1.0,
                        dJrzdsigmaTmp.data(), nJ, tdata->dsigmazdp, nz, 1.0,
                        tdata->dJzdp, nJ);
        }

        amici_dgemm(AMICI_BLAS_ColMajor, AMICI_BLAS_NoTrans, AMICI_BLAS_NoTrans,
                    nJ, rdata->nplist, nz, 1.0, dJzdzTmp.data(), nJ, tdata->dzdp, nz,
                    1.0, tdata->dJzdp, nJ);

        for (int iJ = 0; iJ < nJ; ++iJ) {
            for (int iz = 0; iz < nz; ++iz) {
                dJzdsigmaTmp[iJ + iz * nJ] =
                    tdata->dJzdsigma[izt + (iJ + iz * nJ) * nztrue];
            }
        }

        amici_dgemm(AMICI_BLAS_ColMajor, AMICI_BLAS_NoTrans, AMICI_BLAS_NoTrans,
                    nJ, rdata->nplist, nz, 1.0, dJzdsigmaTmp.data(), nJ,
                    tdata->dsigmazdp, nz, 1.0, tdata->dJzdp, nJ);
    }
}

/** Sensitivity of event-resolved measurement negative log-likelihood Jz w.r.t.
 * state variables
 * @param[in] ie event index @type int
 * @param[in,out] tdata pointer to temp data object @type TempData
 * @param[in] edata pointer to experimental data object @type ExpData
 */
void Model::fdJzdx(const int ie, TempData *tdata, const ExpData *edata) {
    // dJzdz         nztrue x nJ x nz
    // dzdx          nz x nx
    // dJzdx         nmaxevent x nJ x nx

    std::vector<double> multResult(nJ * nx, 0);
    for (int izt = 0; izt < nztrue; ++izt) {
        if (amiIsNaN(
                edata->mz[tdata->nroots[ie] + izt * tdata->rdata->nmaxevent]))
            continue;

        // copy current (izt) dJzdz slice
        // dJzdzTmp     nJ x nz
        if (tdata->t < tdata->rdata->ts[tdata->rdata->nt - 1]) {
            for (int iJ = 0; iJ < nJ; ++iJ)
                for (int iz = 0; iz < nz; ++iz)
                    dJzdzTmp[iJ + iz * nJ] =
                        tdata->dJzdz[izt + (iJ + iz * nJ) * nztrue];

            amici_dgemm(AMICI_BLAS_ColMajor, AMICI_BLAS_NoTrans,
                        AMICI_BLAS_NoTrans, nJ, nx, nz, 1.0, dJzdzTmp.data(), nJ,
                        tdata->dzdx, nz, 1.0, multResult.data(), nJ);
        } else {
            for (int iJ = 0; iJ < nJ; ++iJ) {
                for (int iz = 0; iz < nz; ++iz) {
                    dJzdzTmp[iJ + iz * nJ] =
                        tdata->dJrzdz[izt + (iJ + iz * nJ) * nztrue];
                }
            }

            amici_dgemm(AMICI_BLAS_ColMajor, AMICI_BLAS_NoTrans,
                        AMICI_BLAS_NoTrans, nJ, nx, nz, 1.0, dJzdzTmp.data(), nJ,
                        tdata->drzdx, nz, 1.0, multResult.data(), nJ);
        }
    }
    for (int iJ = 0; iJ < nJ; ++iJ)
        for (int ix = 0; ix < nx; ++ix)
            tdata->dJzdx[tdata->nroots[ie] +
                         (iJ + ix * nJ) * tdata->rdata->nmaxevent] =
                multResult[iJ + ix * nJ];
}

/** initialization of model properties
 * @param[in] udata pointer to user data object @type UserData
 * @param[out] tdata pointer to temp data object @type TempData
 */
void Model::initialize(const UserData *udata, TempData *tdata) {

    initializeStates(udata->x0data, tdata);
    
    fdx0(tdata->x, tdata->dx, tdata);
    
    initHeaviside(tdata);
    
}

/** initialization of initial states
 * @param[in] x0data array with initial state values @type double
 * @param[out] tdata pointer to temp data object @type TempData
 */
void Model::initializeStates(const double *x0data, TempData *tdata) {

    if (!x0data) {
        fx0(tdata->x, tdata);
    } else {
        realtype *x_tmp = NV_DATA_S(tdata->x);
        if (!x_tmp)
            throw NullPointerException("x_tmp");

        for (int ix = 0; ix < nx; ix++) {
            x_tmp[ix] = (realtype)x0data[ix];
        }
    }
}

/**
 * initHeaviside initialises the heaviside variables h at the intial time t0
 * heaviside variables activate/deactivate on event occurences
 *
 * @param[out] tdata pointer to the temporary data struct @type TempData
 */
void Model::initHeaviside(TempData *tdata) {
    
    froot(tdata->t, tdata->x, tdata->dx, tdata->rootvals, tdata);

    for (int ie = 0; ie < ne; ie++) {
        if (tdata->rootvals[ie] < 0) {
            tdata->h[ie] = 0.0;
        } else if (tdata->rootvals[ie] == 0) {
            throw AmiException("Simulation started in an event. This could lead to "
                               "unexpected results, aborting simulation! Please "
                               "specify an earlier simulation start via "
                               "@amimodel.t0");
        } else {
            tdata->h[ie] = 1.0;
        }
    }
}
    
    /** Initial states
     * @param[in] udata object with user input
     **/
    virtual void fx0(AmiVector x, UserData *udata) {
        x.reset();
        model_x0(x.data(),udata->tstart(),udata->p(),udata->k());
    };

    /** Initial value for initial state sensitivities
     * @param[in] udata object with user input
     **/
    virtual void fsx0(AmiVectorArray sx, const AmiVector x, const UserData *udata) {
        sx.reset();
        for(int ip = 0; ip<udata->nplist(); ip++)
            model_sx0(sx.data(ip),udata->tstart(),x.data(),udata->p(),udata->k(),udata->plist(ip));
    }
    
    /** Sensitivity of event timepoint, total derivative
     * @param[in] ie event index
     * @param[in] udata object with user input
     */
    virtual void fstau(const int ie,const realtype t, const AmiVector x, const AmiVectorArray sx, const UserData *udata) {
        std::fill(stau.begin(),stau.end(),0.0);
        for(int ip = 0; ip < udata->nplist(); ip++){
            model_stau(stau.data(),t,x.data(),udata->p(),udata->k(),sx.data(ip),udata->plist(ip),ie);
        }
    }
    
    /** Observables / measurements
     * @param[in] it timepoint index
     * @param[out] rdata pointer to return data object
     * @param[in] udata object with user input
     */
    virtual void fy(int it, ReturnData *rdata, const UserData *udata) {
        const realtype t = gett(it,rdata,udata);
        getx(it,rdata,udata);
        std::fill(y.begin(),y.end(),0.0);
        model_y(y.data(),t,x.data(),udata->p(),udata->k());
        for(int iy; iy < ny; iy++)
            rdata->y[it + udata->nt()*iy] = y.at(iy);
    }
    
    /** partial derivative of observables y w.r.t. model parameters p
     * @param[in] udata object with user input
     */
    virtual void fdydp(const int it, ReturnData *rdata, const UserData *udata) {
        const realtype t = gett(it,rdata,udata);
        getx(it,rdata,udata);
        std::fill(dydp.begin(),dydp.end(),0.0);
        for(int ip = 0; ip < udata->nplist(); ip++){
            model_dydp(dydp.data(),t,x.data(),udata->p(),udata->k(),udata->plist(ip));
        }
    }
    
    /** partial derivative of observables y w.r.t. state variables x
     const UserData *udata
     */
    virtual void fdydx(const int it, ReturnData *rdata, const UserData *udata) {
        const realtype t = gett(it,rdata,udata);
        getx(it,rdata,udata);
        std::fill(dydx.begin(),dydx.end(),0.0);
        model_dydx(dydx.data(),t,x.data(),udata->p(),udata->k());
    }
    
    /** Event-resolved output
     * @param[in] nroots number of events for event index
     * @param[out] rdata pointer to return data object
     * @param[in] udata object with user input
     */
    virtual void fz(const int nroots, const realtype t, const AmiVector x, ReturnData *rdata,
                    const UserData *udata) {
        std::vector<double> zreturn(nz,0.0);
        model_z(zreturn.data(),t,x.data(),udata->p(),udata->k());
        for(int iz; iz < nz; iz++) {
            rdata->z[nroots+udata->nme()*iz] = zreturn.at(iz);
        }
    }
    
    /** Sensitivity of z, total derivative
     * @param[in] nroots number of events for event index
     * @param[out] rdata pointer to return data object
     * @param[in] udata object with user input
     */
    virtual void fsz(const int nroots, const realtype t, const AmiVector x, const AmiVectorArray sx, ReturnData *rdata,
                     const UserData *udata) {
        for(int ip; ip < udata->nplist();  ip++ ){
            std::vector<double> szreturn(nz,0.0);
            model_sz(szreturn.data(),t,x.data(),udata->p(),udata->k(),sx.data(ip),udata->plist(ip));
            for(int iz; iz < nz; iz++) {
                rdata->sz[nroots+udata->nme()*(ip*nz + iz)] = szreturn.at(iz);
            }
        }
    }
    
    /** Event root function of events (equal to froot but does not include
     * non-output events)
     * @param[in] nroots number of events for event index
     * @param[in] udata object with user input
     * @param[out] rdata pointer to return data object
     */
    virtual void frz(const int nroots, const realtype t, const AmiVector x, ReturnData *rdata,
                     const UserData *udata) {
        std::vector<double> rzreturn(nz,0.0);
        model_rz(rzreturn.data(),t,x.data(),udata->p(),udata->k());
        for(int iz; iz < nz; iz++) {
            rdata->rz[nroots+udata->nme()*iz] = rzreturn.at(iz);
        }
    }
    
    /** Sensitivity of rz, total derivative
     * @param[in] nroots number of events for event index
     * @param[out] rdata pointer to return data object
     * @param[in] udata object with user input
     */
    virtual void fsrz(const int nroots, const realtype t, const AmiVector x, const AmiVectorArray sx, ReturnData *rdata,
                      const UserData *udata) {
        for(int ip; ip < udata->nplist();  ip++ ){
            std::vector<double> srzreturn(nz,0.0);
            model_srz(srzreturn.data(),t,x.data(),udata->p(),udata->k(),sx.data(ip),udata->plist(ip));
            for(int iz; iz < nz; iz++) {
                rdata->srz[nroots+udata->nme()*(ip*nz + iz)] = srzreturn.at(iz);
            }
        }
    }
    
    /** partial derivative of event-resolved output z w.r.t. to model parameters p
     * @param[in] udata object with user input
     */
    virtual void fdzdp(const realtype t, const AmiVector x, const UserData *udata) {
        std::fill(dzdp.begin(),dzdp.end(),0.0);
        for(int ip = 0; ip < udata->nplist(); ip++){
            model_dzdp(dzdp.data(),t,x.data(),udata->p(),udata->k(),udata->plist(ip));
        }
    }
    
    /** partial derivative of event-resolved output z w.r.t. to model states x
     * @param[in] udata object with user input
     */
    virtual void fdzdx(const realtype t, const AmiVector x, const UserData *udata) {
        std::fill(dzdx.begin(),dzdx.end(),0.0);
        model_dzdx(dzdx.data(),t,x.data(),udata->p(),udata->k());
    }
    
    /** Sensitivity of event-resolved root output w.r.t. to model parameters p
     * @param[in] udata object with user input
     */
    virtual void fdrzdp(const realtype t, const AmiVector x, const UserData *udata) {
        std::fill(drzdp.begin(),drzdp.end(),0.0);
        for(int ip = 0; ip < udata->nplist(); ip++){
            model_drzdp(drzdp.data(),t,x.data(),udata->p(),udata->k(),udata->plist(ip));
        }
    }
    
    /** Sensitivity of event-resolved measurements rz w.r.t. to model states x
     * @param[in] udata object with user input
     */
    virtual void fdrzdx(const realtype t, const AmiVector x, const UserData *udata) {
        std::fill(drzdx.begin(),drzdx.end(),0.0);
        model_drzdx(drzdx.data(),t,x.data(),udata->p(),udata->k());
    }
    
    /** State update functions for events
     * @param[in] ie event index
     * @param[in] udata object with user input
     */
    virtual void fdeltax(const int ie, const realtype t, const AmiVector x,
                         const AmiVector xdot, const AmiVector xdot_old, const UserData *udata) {
        std::fill(deltax.begin(),deltax.end(),0.0);
        model_deltax(deltax.data(),t,x.data(),udata->p(),udata->k(),ie,xdot.data(),xdot_old.data());
    }
    
    /** Sensitivity update functions for events, total derivative
     * @param[in] ie event index
     * @param[in] udata object with user input
     */
    virtual void fdeltasx(const int ie, const realtype t, const AmiVector x, const AmiVectorArray sx,
                          const AmiVector xdot, const AmiVector xdot_old, const UserData *udata) {
        std::fill(deltasx.begin(),deltasx.end(),0.0);
        for(int ip = 0; ip < udata->nplist(); ip++)
            model_deltasx(deltasx.data(),t,x.data(),udata->p(),udata->k(),
                          udata->plist(ip),ie,xdot.data(),xdot_old.data(),sx.data(ip),stau.data());
    }
    
    /** Adjoint state update functions for events
     * @param[in] ie event index
     * @param[in] udata object with user input
     */
    virtual void fdeltaxB(const int ie, const realtype t, const AmiVector x, const AmiVector xB,
                          const AmiVector xdot, const AmiVector xdot_old, const UserData *udata) {
        std::fill(deltaxB.begin(),deltaxB.end(),0.0);
        model_deltaxB(deltaxB.data(),t,x.data(),udata->p(),udata->k(),ie,xdot.data(),xdot_old.data(),xB.data());
    }
    
    /** Quadrature state update functions for events
     * @param[in] ie event index
     * @param[in] udata object with user input
     */
    virtual void fdeltaqB(const int ie, const realtype t, const AmiVector x, const AmiVector xB,
                          const AmiVector xdot, const AmiVector xdot_old, const AmiVector qBdot, const UserData *udata) {
        std::fill(deltaqB.begin(),deltaqB.end(),0.0);
        for(int ip = 0; ip < udata->nplist(); ip++)
            model_deltaqB(deltaqB.data(),t,x.data(),udata->p(),udata->k(),
                          udata->plist(ip),ie,xdot.data(),xdot_old.data(),xB.data(),qBdot.data());
    }
    
    /** Standard deviation of measurements
     * @param[in] udata object with user input
     */
    virtual void fsigma_y(const realtype t, const UserData *udata) {
        std::fill(sigmay.begin(),sigmay.end(),0.0);
        model_sigma_y(sigmay.data(),t,udata->p(),udata->k());
    }
    
    /** partial derivative of standard deviation of measurements w.r.t. model
     * @param[in] udata object with user input
     */
    virtual void fdsigma_ydp(const realtype t, const UserData *udata) {
        std::fill(dsigmaydp.begin(),dsigmaydp.end(),0.0);
        for(int ip = 0; ip < udata->nplist(); ip++)
            model_dsigma_ydp(dsigmaydp.data(),t,udata->p(),udata->k(),udata->plist(ip));
    }
    
    /** Standard deviation of events
     * @param[in] udata object with user input
     */
    virtual void fsigma_z(const realtype t, const UserData *udata) {
        std::fill(sigmaz.begin(),sigmaz.end(),0.0);
        model_sigma_z(sigmaz.data(),t,udata->p(),udata->k());
    }
    
    /** Sensitivity of standard deviation of events measurements w.r.t. model parameters p
     * @param[in] udata object with user input
     */
    virtual void fdsigma_zdp(const realtype t, const UserData *udata) {
        std::fill(dsigmazdp.begin(),dsigmazdp.end(),0.0);
        for(int ip = 0; ip < udata->nplist(); ip++)
            model_dsigma_zdp(dsigmazdp.data(),t,udata->p(),udata->k(),udata->plist(ip));
    }
    
    /** negative log-likelihood of measurements y
     * @param[in] it timepoint index
     * @param[in] udata object with user input
     * @param[in] rdata pointer to return data object
     * @param[in] edata pointer to experimental data object
     */
    virtual void fJy(std::vector<double> Jy,const int it, const ReturnData *rdata, const UserData *udata, const ExpData *edata) {
        std::vector<double> nllh(nJ,0.0);
        gety(it,rdata,udata);
        getmy(it,edata,udata);
        for(int iytrue = 0; iytrue < nytrue; iytrue++){
            if(!amiIsNaN(edata->my[iytrue* udata->nt()+it])){
                std::fill(nllh.begin(),nllh.end(),0.0);
                model_Jy(nllh.data(),udata->p(),udata->k(),y.data(),sigmay.data(),my.data());
                for(int iJ = 0; iJ < nJ; iJ++){
                    Jy.at(iJ) += nllh.at(iJ);
                }
            }
        }
    }
    
    /** negative log-likelihood of event-resolved measurements z
     * @param[in] nroots event index
     * @param[in] udata object with user input
     * @param[in] rdata pointer to return data object
     * @param[in] edata pointer to experimental data object
     */
    virtual void fJz(std::vector<double> Jz, const int nroots, const ReturnData *rdata, const UserData *udata, const ExpData *edata) {
        std::vector<double> nllh(nJ,0.0);
        getz(nroots,rdata,udata);
        getmz(nroots,edata,udata);
        for(int iztrue = 0; iztrue < nztrue; iztrue++){
            if(!amiIsNaN(edata->mz[nroots + udata->nme()*iztrue])){
                std::fill(nllh.begin(),nllh.end(),0.0);
                model_Jz(nllh.data(),udata->p(),udata->k(),z.data(),sigmaz.data(),mz.data());
                for(int iJ = 0; iJ < nJ; iJ++){
                    Jz.at(iJ) += nllh.at(iJ);
                }
            }
        }
    }
    
    /** regularization of negative log-likelihood with roots of event-resolved
     * measurements rz
     * @param[in] nroots event index
     * @param[in] udata object with user input
     * @param[in] rdata pointer to return data object
     * @param[in] edata pointer to experimental data object
     */
    virtual void fJrz(std::vector<double> Jz, const int nroots, const ReturnData *rdata, const UserData *udata, const ExpData *edata) {
        std::vector<double> nllh(nJ,0.0);
        getrz(nroots,rdata,udata);
        for(int iztrue = 0; iztrue < nztrue; iztrue++){
            if(!amiIsNaN(edata->mz[nroots + udata->nme()*iztrue])){
                std::fill(nllh.begin(),nllh.end(),0.0);
                model_Jrz(nllh.data(),udata->p(),udata->k(),rz.data(),sigmaz.data());
                for(int iJ = 0; iJ < nJ; iJ++){
                    Jz.at(iJ) += nllh.at(iJ);
                }
            }
        }
    }
    
    /** partial derivative of time-resolved measurement negative log-likelihood Jy
     * @param[in] it timepoint index
     * @param[in] udata object with user input
     * @param[in] rdata pointer to return data object
     * @param[in] edata pointer to experimental data object
     */
    virtual void fdJydy(const int it, const ReturnData *rdata,
                        const UserData *udata, const ExpData *edata) {
        gety(it,rdata,udata);
        getmy(it,edata,udata);
        std::vector<double> dJydy_slice(nJ*nytrue, 0.0);
        std::fill(dJydy.begin(),dJydy.end(),0.0);
        for(int iytrue = 0; iytrue < nytrue; iytrue++){
            if(!amiIsNaN(edata->my[iytrue* udata->nt()+it])){
                std::fill(dJydy_slice.begin(),dJydy_slice.end(),0.0);
                model_dJydy(dJydy_slice.data(),udata->p(),udata->k(),y.data(),sigmay.data(),my.data());
                // TODO: fix slicing here such that slicing is no longer necessary in sJy
                for(int iJ = 0; iJ < nJ; iJ++){
                    for(int iy = 0; iy < ny; iy++ )
                        dJydy.at(iytrue+(iJ+iy*nJ)) = dJydy_slice.at(iJ+iy*nJ);
                }
            }
        }
    }
    
    /** Sensitivity of time-resolved measurement negative log-likelihood Jy
     * w.r.t. standard deviation sigma
     * @param[in] it timepoint index
     * @param[in] udata object with user input
     * @param[in] rdata pointer to return data object
     * @param[in] edata pointer to experimental data object
     */
    virtual void fdJydsigma(const int it, const ReturnData *rdata,
                            const UserData *udata, const ExpData *edata) {
        gety(it,rdata,udata);
        getmy(it,edata,udata);
        std::vector<double> dJydsigma_slice(nJ*nytrue, 0.0);
        std::fill(dJydsigma.begin(),dJydsigma.end(),0.0);
        for(int iytrue = 0; iytrue < nytrue; iytrue++){
            if(!amiIsNaN(edata->my[iytrue* udata->nt()+it])){
                std::fill(dJydsigma_slice.begin(),dJydsigma_slice.end(),0.0);
                model_dJydsigma(dJydsigma_slice.data(),udata->p(),udata->k(),y.data(),sigmay.data(),my.data());
                // TODO: fix slicing here such that slicing is no longer necessary in sJy
                for(int iJ = 0; iJ < nJ; iJ++){
                    for(int iy = 0; iy < ny; iy++ )
                        dJydsigma.at(iytrue+(iJ+iy*nJ)) = dJydsigma_slice.at(iJ+iy*nJ);
                }
            }
        }
    }
    
    /** partial derivative of event measurement negative log-likelihood Jz
     * @param[in] nroots event index
     * @param[in] udata object with user input
     * @param[in] rdata pointer to return data object
     * @param[in] edata pointer to experimental data object
     */
    virtual void fdJzdz(const int nroots, const ReturnData *rdata,
                        const UserData *udata, const ExpData *edata) {
        getz(nroots,rdata,udata);
        getmz(nroots,edata,udata);
        std::vector<double> dJzdz_slice(nJ*nztrue, 0.0);
        std::fill(dJzdz.begin(),dJzdz.end(),0.0);
        for(int iztrue = 0; iztrue < nztrue; iztrue++){
            if(!amiIsNaN(edata->mz[iztrue*udata->nme()+nroots])){
                std::fill(dJzdz_slice.begin(),dJzdz_slice.end(),0.0);
                model_dJzdz(dJzdz_slice.data(),udata->p(),udata->k(),z.data(),sigmaz.data(),mz.data());
                // TODO: fix slicing here such that slicing is no longer necessary in sJz
                for(int iJ = 0; iJ < nJ; iJ++){
                    for(int iz = 0; iz < nz; iz++ )
                        dJzdz.at(iztrue+(iJ+iz*nJ)) = dJzdz_slice.at(iJ+iz*nJ);
                }
            }
        }
    }
    
    /** Sensitivity of event measurement negative log-likelihood Jz
     * w.r.t. standard deviation sigmaz
     * @param[in] nroots event index
     * @param[in] udata object with user input
     * @param[in] rdata pointer to return data object
     * @param[in] edata pointer to experimental data object
     */
    virtual void fdJzdsigma(const int nroots, const ReturnData *rdata,
                            const UserData *udata, const ExpData *edata) {
        getz(nroots,rdata,udata);
        getmz(nroots,edata,udata);
        std::vector<double> dJzdsigma_slice(nJ*nztrue, 0.0);
        std::fill(dJzdsigma.begin(),dJzdsigma.end(),0.0);
        for(int iztrue = 0; iztrue < nztrue; iztrue++){
            if(!amiIsNaN(edata->mz[iztrue*udata->nme()+nroots])){
                std::fill(dJzdsigma_slice.begin(),dJzdsigma_slice.end(),0.0);
                model_dJzdsigma(dJzdsigma_slice.data(),udata->p(),udata->k(),z.data(),sigmaz.data(),mz.data());
                // TODO: fix slicing here such that slicing is no longer necessary in sJz
                for(int iJ = 0; iJ < nJ; iJ++){
                    for(int iz = 0; iz < nz; iz++ )
                        dJzdsigma.at(iztrue+(iJ+iz*nJ)) = dJzdsigma_slice.at(iJ+iz*nJ);
                }
            }
        }
    }
    
    /** partial derivative of event measurement negative log-likelihood Jz
     * @param[in] nroots event index
     * @param[in] udata object with user input
     * @param[in] rdata pointer to return data object
     * @param[in] edata pointer to experimental data object
     */
    virtual void fdJrzdz(const int nroots, const ReturnData *rdata,
                         const UserData *udata, const ExpData *edata) {
        getrz(nroots,rdata,udata);
        getmz(nroots,edata,udata);
        std::vector<double> dJrzdz_slice(nJ*nztrue, 0.0);
        std::fill(dJrzdz.begin(),dJrzdz.end(),0.0);
        for(int iztrue = 0; iztrue < nztrue; iztrue++){
            if(!amiIsNaN(edata->mz[iztrue*udata->nme()+nroots])){
                std::fill(dJrzdz_slice.begin(),dJrzdz_slice.end(),0.0);
                model_dJrzdz(dJrzdz_slice.data(),udata->p(),udata->k(),rz.data(),sigmaz.data());
                // TODO: fix slicing here such that slicing is no longer necessary in sJz
                for(int iJ = 0; iJ < nJ; iJ++){
                    for(int iz = 0; iz < nz; iz++)
                        dJrzdz.at(iztrue+(iJ+iz*nJ)) = dJrzdz_slice.at(iJ+iz*nJ);
                }
            }
        }
    }
    
    /** Sensitivity of event measurement negative log-likelihood Jz
     * w.r.t. standard deviation sigmaz
     * @param[in] nroots event index
     * @param[in] udata object with user input
     * @param[in] rdata pointer to return data object
     * @param[in] edata pointer to experimental data object
     */
    virtual void fdJrzdsigma(const int nroots,const ReturnData *rdata,
                             const UserData *udata, const ExpData *edata) {
        getrz(nroots,rdata,udata);
        std::vector<double> dJrzdsigma_slice(nJ*nztrue, 0.0);
        std::fill(dJrzdsigma.begin(),dJrzdsigma.end(),0.0);
        for(int iztrue = 0; iztrue < nztrue; iztrue++){
            if(!amiIsNaN(edata->mz[iztrue*udata->nme()+nroots])){
                std::fill(dJrzdsigma_slice.begin(),dJrzdsigma_slice.end(),0.0);
                model_dJrzdsigma(dJrzdsigma_slice.data(),udata->p(),udata->k(),rz.data(),sigmaz.data());
                // TODO: fix slicing here such that slicing is no longer necessary in sJz
                for(int iJ = 0; iJ < nJ; iJ++){
                    for(int iz = 0; iz < nz; iz++ )
                        dJrzdsigma.at(iztrue+(iJ+iz*nJ)) = dJrzdsigma_slice.at(iJ+iz*nJ);
                }
            }
        }
    }
    
    /**
     * @brief Recurring terms in xdot
     * @param[in] t timepoint
     * @param[in] x Vector with the states
     * @param[in] udata object with user input
     */
    void fw(const realtype t, const N_Vector x, const UserData *udata) {
        std::fill(w.begin(),w.end(),0.0);
        model_w(w.data(),t,N_VGetArrayPointer(x),udata->p(),udata->k());
    }
    
    /**
     * @brief Recurring terms in xdot, parameter derivative
     * @param[in] t timepoint
     * @param[in] x Vector with the states
     * @param[in] udata object with user input
     */
    void fdwdp(const realtype t, const N_Vector x, const UserData *udata) {
        fw(t,x,udata);
        std::fill(dwdp.begin(),dwdp.end(),0.0);
        model_dwdp(dwdp.data(),t,N_VGetArrayPointer(x),udata->p(),udata->k(),w.data());
    }
    
    /**
     * @brief Recurring terms in xdot, state derivative
     * @param[in] t timepoint
     * @param[in] x Vector with the states @type N_Vector
     * @param[in] udata object with user input
     */
    void fdwdx(const realtype t, const N_Vector x, const UserData *udata) {
        fw(t,x,udata);
        std::fill(dwdx.begin(),dwdx.end(),0.0);
        model_dwdx(dwdx.data(),t,N_VGetArrayPointer(x),udata->p(),udata->k(),w.data());
    }
    
    /** create my slice at timepoint
     * @param[in] it timepoint index
     * @param[in] udata object with user input
     * @param[in] edata pointer to experimental data object
     */
    void getmy(const int it, const ExpData *edata, const UserData *udata) {
        for(int iytrue = 0; iytrue < nytrue; iytrue++){
            my.at(iytrue) = edata->my[it + udata->nt()*iytrue];
        }
    }
    
    /** create y slice at timepoint
     * @param[in] it timepoint index
     * @param[in] udata object with user input
     * @param[in] rdata pointer to return data object
     */
    void gety(const int it, const ReturnData *rdata, const UserData *udata) {
        for(int iy = 0; iy < ny; iy++){
            y.at(iy) = rdata->y[it + udata->nt()*iy];
        }
    }
    
    /** create x slice at timepoint
     * @param[in] it timepoint index
     * @param[in] udata object with user input
     * @param[in] rdata pointer to return data object
     */
    void getx(const int it, const ReturnData *rdata, const UserData *udata) {
        for(int ix = 0; ix < nx; ix++){
            x.at(ix) = rdata->x[it + udata->nt()*ix];
        }
    }
    
    /** create t  at timepoint
     * @param[in] it timepoint index
     * @param[in] udata object with user input
     * @param[in] rdata pointer to return data object
     */
    const realtype gett(const int it, const ReturnData *rdata, const UserData *udata) const {
        return rdata->ts[it];;
    }
    
    /** create mz slice at event
     * @param[in] nroots event occurence
     * @param[in] udata object with user input
     * @param[in] edata pointer to experimental data object
     */
    void getmz(const int nroots, const ExpData *edata, const UserData *udata) {
        for(int iztrue = 0; iztrue < nztrue; iztrue++){
            mz.at(iztrue) = edata->mz[nroots + udata->nme()*iztrue];
        }
    }
    
    /** create z slice at event
     * @param[in] nroots event occurence
     * @param[in] udata object with user input
     * @param[in] rdata pointer to return data object
     */
    void getz(const int nroots, const ReturnData *rdata, const UserData *udata) {
        for(int iz = 0; iz < nz; iz++){
            z.at(iz) = rdata->z[nroots+udata->nme()*iz];
        }
    }
    
    /** create rz slice at event
     * @param[in] nroots event occurence
     * @param[in] udata object with user input
     * @param[in] rdata pointer to return data object
     */
    void getrz(const int nroots, const ReturnData *rdata, const UserData *udata) {
        for(int iz = 0; iz < nz; iz++){
            rz.at(iz) = rdata->rz[nroots+udata->nme()*iz];
        }
    }
    
    

} // namespace amici
