#ifndef AMICI_STEADYSTATEPROBLEM_H
#define AMICI_STEADYSTATEPROBLEM_H

#include "amici/defines.h"
#include "amici/vector.h"
#include "amici/solver_cvodes.h"
#include "amici/forwardproblem.h"
#include <amici/newton_solver.h>

#include <nvector/nvector_serial.h>

#include <functional>
#include <memory>

namespace amici {

class ExpData;
class Solver;
class Model;

/**
 * @brief The SteadystateProblem class solves a steady-state problem using
 * Newton's method and falls back to integration on failure.
 */
class SteadystateProblem {
  public:
    /**
     * @brief constructor
     * @param solver Solver instance
     * @param model Model instance
     */
    explicit SteadystateProblem(const Solver &solver,
                                const Model &model);

    /**
     * Tries to determine the steady state of the ODE system by a Newton
     * solver, uses forward integration, if the Newton solver fails,
     * restarts Newton solver, if integration fails.
     * Computes steady state sensitivities
     *
     * @param solver pointer to the solver object
     * @param model pointer to the model object
     * @param it integer with the index of the current time step
     */
    void workSteadyStateProblem(Solver *solver, Model *model, int it);

    /**
     * Handles the computation of the steady state, throws an AmiException,
     * if no steady state was found
     *
     * @param solver pointer to the solver object
     * @param newtonSolver pointer to the newtonSolver solver object
     * @param model pointer to the model object
     * @param it integer with the index of the current time step
     */
    void findSteadyState(Solver *solver,
                         NewtonSolver *newtonSolver,
                         Model *model, int it);

    /**
     * Tries to determine the steady state by using Newton's method
     *
     * @param newtonSolver pointer to the newtonSolver solver object
     * @param model pointer to the model object
     * @param newton_retry bool flag indicating whether being relaunched
     */
    void findSteadyStateByNewtonsMethod(NewtonSolver *newtonSolver,
                                        Model *model,
                                        bool newton_retry);

    /**
     * Tries to determine the steady state by using forward simulation
     *
     * @param solver pointer to the solver object
     * @param model pointer to the model object
     * @param it integer with the index of the current time step
     */
    void findSteadyStateBySimulation(Solver *solver,
                                     Model *model,
                                     int it);

    /**
     * Stores state and throws error message if steady state computaiton failed
     *
     * @param solver pointer to the solver object
     * @param model pointer to the model object
     */
    [[noreturn]] void handleSteadyStateFailure(const Solver *solver,
                                               Model *model);

    /**
     * Assembles the error message to be thrown.
     *
     * @param errorString string with error message
     * @param status Entry of steady_state_staus to be processed
     * @return errorString updated string with error message
     */
    std::string writeErrorString(std::string errorString,
                                 SteadyStateStatus status) const;

    /**
     * Checks depending on the status of the Newton solver,
     * solver settings, and the model, whether state sensitivities
     * still need to be computed via a linear system solve or stored
     *
     * @param model pointer to the model object
     * @param solver pointer to the solver object
     * @param it integer with the index of the current time step
     * @param context SteadyStateContext giving the situation for the flag
     * @return flag telling how to process state sensis
     */
    bool getSensitivityFlag(const Model *model, const Solver *solver, int it,
                            SteadyStateContext context);

    /**
     * Integrates over the adjoint state backward in time by solving a linear
     * system of equations, which gives the analytical solution.
     * Computes the gradient via adjoint steady state sensitivities
     *
     * @param solver pointer to the solver object
     * @param model pointer to the model object
     */
    void workSteadyStateBackwardProblem(Solver *solver, Model *model);

    /**
     * Computes the weighted root mean square of xdot
     * the weights are computed according to x:
     * w_i = 1 / ( rtol * x_i + atol )
     *
     * @param x current state
     * @param xdot current rhs
     * @param atol absolute tolerance
     * @param rtol relative tolerance
     * @return root-mean-square norm
     */
    realtype getWrmsNorm(AmiVector const &x,
                         AmiVector const &xdot,
                         realtype atol,
                         realtype rtol);

    /**
     * Checks convergence for state and respective sensitivities
     *
     * @param solver Solver instance
     * @param model instance
     * @return boolean indicating convergence
     */
    bool checkConvergence(const Solver *solver,
                          Model *model);

    /**
     * Runs the Newton solver iterations and checks for convergence to steady
     * state
     *
     * @param model pointer to the model object
     * @param newtonSolver pointer to the NewtonSolver object @type
     * NewtonSolver
     * @param newton_retry flag indicating if Newton solver is rerun
     */
    void applyNewtonsMethod(Model *model, NewtonSolver *newtonSolver,
                            bool newton_retry);

    /**
     * Forward simulation is launched, if Newton solver fails in first try
     *
     * @param solver pointer to the solver object
     * @param model pointer to the model object
     */
    void getSteadystateSimulation(Solver *solver, Model *model);

    /**
     * initialize CVodeSolver instance for preequilibration simulation
     *
     * @param solver pointer to the solver object
     * @param model pointer to the model object
     * @param integrateForwardSensis flag switching on integration with FSA
     * @return solver instance
     */
    std::unique_ptr<Solver> createSteadystateSimSolver(const Solver *solver,
                                                       Model *model,
                                                       bool integrateForwardSensis) const;

    /**
     * @brief store carbon copy of current simulation state variables as SimulationState
     * @param model model carrying the ModelState to be used
     * @param storesensi flag to enable storage of sensitivities
     */
    void storeSimulationState(Model *model, bool storesensi);

    /**
     * @brief returns the stored SimulationState
     * @return stored SimulationState
     */
    const SimulationState &getFinalSimulationState() const {
        return state;
    };

    /**
    * @brief returns the quadratures from pre- or postequilibration
    * @return xQB Vector with quadratures
    */
    const AmiVector &getEquilibrationQuadratures() const {
        return xQB;
    }
    /**
     * @brief returns state at steadystate
     * @return x
     */
    const AmiVector &getState() const {
        return x;
    };


    /**
     * @brief returns state sensitivity at steadystate
     * @return sx
     */
    const AmiVectorArray &getStateSensitivity() const {
        return sx;
    };

     /**
      * @brief Accessor for dJydx
      * @return dJydx
      */
    std::vector<realtype> const& getDJydx() const {
         return dJydx;
     }

    /**
     * @brief Accessor for run_time of the forward problem
     * @return run_time
     */
    double getCPUTime() const { return cpu_time; }

    /**
     * @brief Accessor for run_time of the backward problem
     * @return run_time
     */
    double getCPUTimeB() const { return cpu_timeB; }

    /**
     * @brief Accessor for steady_state_status
     * @return steady_state_status
     */
    std::vector<SteadyStateStatus> const& getSteadyStateStatus() const
    { return steady_state_status; }

    /**
     * @brief Accessor for t
     * @return t
     */
    realtype getSteadyStateTime() const { return t; }

    /**
     * @brief Accessor for wrms
     * @return wrms
     */
    realtype getResidualNorm() const { return wrms; }

    /**
     * @brief Accessor for numsteps
     * @return numsteps
     */
    const std::vector<int> &getNumSteps() const { return numsteps; }

    /**
     * @brief Accessor for numlinsteps
     * @return numlinsteps
     */
    const std::vector<int> &getNumLinSteps() const { return numlinsteps; }

    /**
     * @brief computes adjoint updates dJydx according to provided model and expdata
     * @param model Model instance
     * @param edata experimental data
     */
    void getAdjointUpdates(Model &model, const ExpData &edata);

    /**
     * @brief computes adjoint updates dJydx according to provided model and expdata
     * @return covergence of steady state solver
     */
    bool checkSteadyStateSuccess() const;

  private:
    /** time variable for simulation steadystate finding */
    realtype t;
    /** newton step */
    AmiVector delta;
    /** error weights */
    AmiVector ewt;
    /** container for relative error calcuation? */
    AmiVector rel_x_newton;
    /** container for absolute error calcuation? */
    AmiVector x_newton;
    /** state vector */
    AmiVector x;
    /** old state vector */
    AmiVector x_old;
    /** differential state vector */
    AmiVector dx;
    /** time derivative state vector */
    AmiVector xdot;
    /** old time derivative state vector */
    AmiVector xdot_old;
    /** state sensitivities */
    AmiVectorArray sx;
    /** state differential sensitivities */
    AmiVectorArray sdx;
    /** adjoint state vector */
    AmiVector xB;
    /** quadrature state vector */
    AmiVector xQB;

    /** maximum number of steps for Newton solver for allocating numlinsteps */
    int maxSteps = 0;

    /** weighted root-mean-square error */
    realtype wrms = NAN;

    /** state derivative of data likelihood
     * (dimension nJ x nx x nt, ordering =?) */
    std::vector<realtype> dJydx;

    SimulationState state;

    /** stores diagnostic information about employed number of steps */
    std::vector<int> numsteps;

    /** stores diagnostic information about employed number of linear steps */
    std::vector<int> numlinsteps;

    /** stores diagnostic information about runtime */
    double cpu_time;

    /** stores diagnostic information about execution success of the different
     * approaches [newton, simulation, newton] (length = 3)
     */
    std::vector<SteadyStateStatus> steady_state_status;

    /** stores diagnostic information about runtime backward */
    double cpu_timeB;
};

} // namespace amici
#endif // STEADYSTATEPROBLEM_H
