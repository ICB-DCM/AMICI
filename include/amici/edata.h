#ifndef AMICI_EDATA_H
#define AMICI_EDATA_H

#include "amici/defines.h"

#include <vector>

namespace amici {

class Model;
class ReturnData;

/** @brief ExpData carries all information about experimental or condition-specific data */
class ExpData {

  public:
    /** default constructor */
    ExpData();

    /**
     * @brief ExpData
     * @param nytrue
     * @param nztrue
     * @param nmaxevent
     */
    ExpData(int nytrue, int nztrue, int nmaxevent);

    /**
     * @brief ExpData
     * @param nytrue
     * @param nztrue
     * @param nmaxevent
     * @param ts
     */
    ExpData(int nytrue, int nztrue, int nmaxevent,
            std::vector<realtype> ts);
    
    
    /**
     * @brief ExpData
     * @param nytrue
     * @param nztrue
     * @param nmaxevent
     * @param ts
     * @param my
     * @param sigmay
     * @param mz
     * @param sigmaz
     */
    ExpData(int nytrue, int nztrue, int nmaxevent,
            std::vector<realtype> ts,
            std::vector<realtype> my,
            std::vector<realtype> sigmay,
            std::vector<realtype> mz,
            std::vector<realtype> sigmaz);

    /**
     * constructor that initializes with Model
     *
     * @param model pointer to model specification object
     */
    ExpData(const Model &model);
    
    /**
     * constructor that initializes with returnData, adds
     *
     * @param rdata return data pointer with stored simulation results
     * @param sigma_y scalar standard deviations for all observables
     * @param sigma_z scalar standard deviations for all event observables
     */
    ExpData(const ReturnData &rdata, realtype sigma_y, realtype sigma_z);
    
    /**
     * constructor that initializes with returnData, adds
     *
     * @param rdata return data pointer with stored simulation results
     * @param sigma_y vector of standard deviations for every observable
     * @param sigma_z vector of standard deviations for every event observable
     */
    ExpData(const ReturnData &rdata, std::vector<realtype> sigma_y, std::vector<realtype> sigma_z);

    /**
     * @brief Copy constructor
     * @param other object to copy from
     */
    ExpData (const ExpData &other);
    

    ~ExpData() = default;
    
    /**
     * number of timepoints
     *
     */
     const int nt() const;

    /**
     * set function that copies data from input to ExpData::ts
     *
     * @param timepoints timepoints
     */
    void setTimepoints(const std::vector<realtype> &ts);
    
    /**
     * get function that copies data from ExpData::ts to output
     *
     * @return ExpData::ts
     */
    std::vector<realtype> getTimepoints() const;
    
    /**
     * get function that returns timepoint at index
     *
     * @param it timepoint index
     * @return timepoint timepoint at index
     */
    realtype getTimepoint(int it) const;
    
    /**
     * set function that copies data from input to ExpData::my
     *
     * @param observedData observed data (dimension: nt x nytrue, row-major)
     */
    void setObservedData(const std::vector<realtype> &observedData);
    
    /**
     * set function that copies observed data for specific observable
     *
     * @param observedData observed data (dimension: nt)
     * @param iy oberved data index
     */
    void setObservedData(const std::vector<realtype> &observedData, int iy);
    
    /**
     * get function that checks whether data at specified indices has been set
     *
     * @param it time index
     * @param iy observable index
     * @return boolean specifying if data was set
     */
    bool isSetObservedData(int it, int iy) const;
    
    /**
     * get function that copies data from ExpData::observedData to output
     *
     * @return observed data (dimension: nt x nytrue, row-major)
     */
    std::vector<realtype> getObservedData() const;
    
    /**
     * get function that returns a pointer to observed data at index
     *
     * @param it timepoint index
     * @return observed data at index (dimension: nytrue)
     */
    const realtype *getObservedDataPtr(int it) const;
    
    /**
     * set function that copies data from input to ExpData::observedDataStdDev
     *
     * @param observedDataStdDev standard deviation of observed data (dimension: nt x nytrue, row-major)
     */
    void setObservedDataStdDev(const std::vector<realtype> &observedDataStdDev);
    
    /**
     * set function that sets all ExpData::observedDataStdDev to the input value
     *
     * @param StdDev standard deviation (dimension: scalar)
     */
    void setObservedDataStdDev(const realtype StdDev);
    
    /**
     * set function that copies standard deviation of observed data for specific observable
     *
     * @param observedDataStdDev standard deviation of observed data (dimension: nt)
     * @param iy observed data index
     */
    void setObservedDataStdDev(const std::vector<realtype> &observedDataStdDev, int iy);
    
    /**
     * set function that sets all standard deviation of a specific observable to the input value
     *
     * @param StdDev standard deviation (dimension: scalar)
     * @param iy observed data index
     */
    void setObservedDataStdDev(const realtype StdDev, int iy);
    
    /**
     * get function that checks whether standard deviation of data at specified indices has been set
     *
     * @param it time index
     * @param iy observable index
     * @return boolean specifying if standard deviation of data was set
     */
    bool isSetObservedDataStdDev(int it, int iy) const;
    
    /**
     * get function that copies data from ExpData::observedDataStdDev to output
     *
     * @return standard deviation of observed data
     */
    std::vector<realtype> getObservedDataStdDev() const;
    
    /**
     * get function that returns a pointer to standard deviation of observed data at index
     *
     * @param it timepoint index
     * @return standard deviation of observed data at index
     */
    const realtype *getObservedDataStdDevPtr(int it) const;
    
    /**
     * set function that copies observed event data from input to ExpData::observedEvents
     *
     * @param observedData observed data (dimension: nmaxevent x nztrue, row-major)
     */
    void setObservedEvents(const std::vector<realtype> &observedEvents);
    
    /**
     * set function that copies observed event data for specific event observable
     *
     * @param observedData observed data (dimension: nmaxevent)
     * @param iz observed event data index
     */
    void setObservedEvents(const std::vector<realtype> &observedEvents, int iz);
    
    /**
     * get function that checks whether event data at specified indices has been set
     *
     * @param ie event index
     * @param iz event observable index
     * @return boolean specifying if data was set
     */
    bool isSetObservedEvents(int ie, int iz) const;
    
    /**
     * get function that copies data from ExpData::mz to output
     *
     * @return observed event data
     */
    std::vector<realtype> getObservedEvents() const;
    
    /**
     * get function that returns a pointer to observed data at ieth occurence
     *
     * @param ie event occurence
     * @return observed event data at ieth occurence
     */
    const realtype *getObservedEventsPtr(int ie) const;
    
    /**
     * set function that copies data from input to ExpData::observedEventsStdDev
     *
     * @param observedEventsStdDev standard deviation of observed event data
     */
    void setObservedEventsStdDev(const std::vector<realtype> &observedEventsStdDev);
    
    /**
     * set function that sets all ExpData::observedDataStdDev to the input value
     *
     * @param StdDev standard deviation (dimension: scalar)
     */
    void setObservedEventsStdDev(const realtype StdDev);
    
    /**
     * set function that copies standard deviation of observed data for specific observable
     *
     * @param observedEventsStdDev standard deviation of observed data (dimension: nmaxevent)
     * @param iy observed data index
     */
    void setObservedEventsStdDev(const std::vector<realtype> &observedEventsStdDev, int iz);
    
    /**
     * set function that sets all standard deviation of a specific observable to the input value
     *
     * @param StdDev standard deviation (dimension: scalar)
     * @param iz observed data index
     */
    void setObservedEventsStdDev(const realtype StdDev, int iz);
    
    /**
     * get function that checks whether standard deviation of even data at specified indices has been set
     *
     * @param ie event index
     * @param iz event observable index
     * @return boolean specifying if standard deviation of event data was set
     */
    bool isSetObservedEventsStdDev(int ie, int iz) const;
    
    /**
     * get function that copies data from ExpData::observedEventsStdDev to output
     *
     * @return standard deviation of observed event data
     */
    std::vector<realtype> getObservedEventsStdDev() const;
    
    /**
     * get function that returns a pointer to standard deviation of observed event data at ieth occurence
     *
     * @param ie event occurence
     * @return standard deviation of observed event data at ieth occurence
     */
    const realtype *getObservedEventsStdDevPtr(int ie) const;
    
    /** number of observables */
    const int nytrue;
    /** number of event observables */
    const int nztrue;
    /** maximal number of event occurences */
    const int nmaxevent;
    
    /** condition-specific parameters of size Model::nk() or empty */
    std::vector<realtype> fixedParameters;
    /** condition-specific parameters for pre-equilibration of size Model::nk() or empty.
     * Overrides Solver::newton_preeq */
    std::vector<realtype> fixedParametersPreequilibration;

protected:
    /** observation timepoints (dimension: nt) */
    std::vector<realtype> ts;
    
    /** observed data (dimension: nt x nytrue, row-major) */
    std::vector<realtype> observedData;
    /** standard deviation of observed data (dimension: nt x nytrue, row-major) */
    std::vector<realtype> observedDataStdDev;
    
    /** observed events (dimension: nmaxevents x nztrue, row-major) */
    std::vector<realtype> observedEvents;
    /** standard deviation of observed events/roots
     * (dimension: nmaxevents x nztrue, row-major)*/
    std::vector<realtype> observedEventsStdDev;
};

} // namespace amici

#endif /* AMICI_EDATA_H */
