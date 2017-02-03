
#ifndef LOCAL_SYSTEM_HH_
#define LOCAL_SYSTEM_HH_

#include <armadillo>

class LinSys;

/** Local system class is meant to be used for local assembly and then pass to global linear system.
 * The key idea is to take care of known solution values (Dirichlet boundary conditions) in a common way.
 * 
 * Usage of the class consists of 3 steps:
 * 1) create local system, set global DoFs.
 * 2) set all known values (Dirichlet BC)
 * 3) set matrix and RHS entries 
 *    (if the entry is on dirichlet row or column, it is now taken care of)
 * 4) eliminate known solution and possibly fix the diagonal entries of the local system, where Dirichlet BC is set
 * 
 */
class LocalSystem
{
friend class LinSys;
protected:
    arma::mat matrix;   ///< local system matrix
    arma::vec rhs;      ///< local system RHS
    
    /// vector of global row indices where the solution is set (dirichlet BC)
    std::vector<unsigned int> global_solution_dofs;
    /// vector of solution values at @p global_solution_rows indices (dirichlet BC)
    std::vector<double> solution;
    /// diagonal values for dirichlet BC rows (set in set_solution)
    std::vector<double> preferred_diag_values;
    
    /**
     * Optimization. Is false if solution (at least one entry) is known.
     */
    bool solution_not_set;
    
public:
    
    std::vector<int> row_dofs;  ///< global row indices
    std::vector<int> col_dofs;  ///< global column indices
    
    /** @brief Constructor.
     * 
     * @p nrows is number of rows of local system
     * @p ncols is number of columns of local system
     */
    LocalSystem(unsigned int nrows, unsigned int ncols);
    
    /// Resets the matrix, RHS, dofs to zero and clears solution settings
    void reset();
    
    const arma::mat& get_matrix() {return matrix;}
    const arma::vec& get_rhs() {return rhs;}
    
    /** @brief Set the position and value of known solution.
     * 
     * @p loc_row is local row index in solution vector
     * @p solution is the values of the solution
     * @p diag_val is preferred diagonal value on the solution row
     */
    void set_solution(unsigned int global_row, double solution_val, double diag_val = 0.0);
    
    /** 
     * When finished with assembly of the local system,
     * this function eliminates all the known dofs.
     * 
     * It is skipped if there is not any solution dof set.
     * 
     * During elimination, the (global) diagonal entries on the rows, where the solution is set, might be zero.
     * Therefore it is necessary to set a proper value to the diagonal entry
     * and respective RHS entry, such that the given solution holds.
     * If preferred diagonal value has been set by @p set_solution then it is used.
     * 
     * Calling this function after the assembly of local system is finished is users's responsibility.
     */
    void eliminate_solution();

    /** @brief Adds a single entry into the local system.
     * 
     * @p row is local row index of local system
     * @p col is local column index of local system
     * @p mat_val is matrix entry value
     * @p rhs_val is RHS entry value
     */
    void add_value(unsigned int row, unsigned int col,
                   double mat_val, double rhs_val);
    
    /** @brief Matrix entry. 
     * Adds a single entry into the local system matrix.
     * 
     * @p row is local row index of local system
     * @p col is local column index of local system
     * @p mat_val is matrix entry value
     */
    void add_value(unsigned int row, unsigned int col,
                   double mat_val);
    
    /** @brief RHS entry.
     * Adds a single entry into the local system RHS.
     * 
     * @p row is local row index of local system
     * @p rhs_val is RHS entry value
     */
    void add_value(unsigned int row, double rhs_val);
};
    
#endif // LOCAL_SYSTEM_HH_