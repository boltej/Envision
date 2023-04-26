#pragma once


//-------------------------------------------------------------------
// PROGRAM: tMatrix.hpp
//
// PURPOSE: Provides general dynamic matrix classes
//
// (Changes: To accomodate  LIBSAPI as __declspec(dllimport) member
//           definitions joined to declarations.
//           
//    Used as DLL you may need to add additional EXPLICIT INSTANCIATIONS. )
// 
//-------------------------------------------------------------------

#pragma once
#include "libs.h"
//#include "Eigen\Dense"
#include "Report.h"

#include <stdlib.h>
//using Eigen::Matrix;
//typedef Matrix<VData, Eigen::Dynamic, Eigen::Dynamic>  MatrixXv;


template <class T>
class LIBSAPI TMatrix
{
protected:
    UINT numRows;
    UINT numCols;
    UINT allocRows;   // number of allocated row ptrs
    UINT m_rowAllocIncr;  // row allocation increment - dynamically calculated

public:
    TMatrix(void) : numRows(0), numCols(0), allocRows(0), m_rowAllocIncr(50) { }
    TMatrix(UINT _numRows, UINT _numCols)
        : numRows(_numRows), numCols(_numCols), allocRows(0), m_rowAllocIncr(50) { }

    virtual ~TMatrix(void) {}

    void SetRowAllocationIncr(UINT rowAllocIncr) { m_rowAllocIncr = rowAllocIncr; }
    UINT GetRowAllocationIncr() { return m_rowAllocIncr; }
    UINT GetRows(void) { return numRows; }
    UINT GetCols(void) { return numCols; }
    UINT GetRowCount(void) { return numRows; }
    UINT GetColCount(void) { return numCols; }

protected:
    std::vector<T> data;
    //T** matrix;         // pointer to an array of pointers to arrays

public:
    //-- constructor------------------------------------------------------------------- 
    TMatrix(UINT _numRows, UINT _numCols)
        : numRows(_numRows)
        , numCols(_numCols)
        , allocRows(0)   // number of allocated row ptrs
    {
    if (numRows <= 0)
        return;

    matrix.resize(numRows);  //matrix = new T * [numRows];
    allocRows = numRows;

    if (matrix.size() != numRows)
        {
        Report::ErrorMsg("TMatrix::TMatrix: Allocation Error");
        return;
        }

    for (UINT i = 0; i < numRows; ++i)
        matrix[i] = std::vector<T>(numCols);
    }


    //-- constructor------------------------------------------------------------------- 
    TMatrix(UINT _numRows, UINT _numCols, T initialValue)
    {
        if (numRows <= 0)
            return;

        matrix = std::vector<T*>(numRows, nullptr); //  new T* [numRows];
        allocRows = numRows;

        if (matrix.size() != numRows)
        {
            Report::ErrorMsg("TMatrix::TMatrix: Allocation Error");
            return;
        }

        for (UINT i = 0; i < numRows; ++i)
            matrix[i] = std::vector<T>(numCols)new T[numCols];

        for (UINT row = 0; row < numRows; row++)
            for (UINT col = 0; col < numCols; col++)
                matrix[row][col] = initialValue;
    }

    //-- constructor------------------------------------------------------------------- 
    TMatrix(void)
        : _Matrix(),
        matrix(NULL)
    { }

    //-- conversion constructor------------------------------------------------------------------- 
    // <> Assumes the arg 'data' is row-by-row
    TMatrix(const T* data, UINT nrows, UINT ncols) : _Matrix()
    {
        ASSERT(NULL != data);

        Resize(numRows, numCols);

        if (data == NULL)
            return;

        for (unsigned int r = 0; r < numRows; ++r)
        {
            memcpy((void*)matrix[r], (void*)&data[r * numCols], numCols * sizeof(T));
        }
    }

    //-- copy constructor -------------------------------------------------------------
    TMatrix(TMatrix& tm) : matrix(NULL)
    {
        //-- set the number of columns, rows 
        Resize(tm.numRows, tm.numCols);

        //-- copy data 
        for (UINT i = 0; i < numRows; i++)
            for (UINT j = 0; j < numCols; j++)
                Set(i, j, tm.Get(i, j));
    }

    //-- destructor -------------------------------------------------------------------- 
    virtual ~TMatrix(void) { Clear(); }

    //----------------------------------------------------------------------------------
    void Clear(void)
    {
        if (matrix == NULL)
            return;

        for (UINT i = 0; i < numRows; i++)
        {
            if (matrix[i] == NULL)
            {
                Report::ErrorMsg("TMatrix::Clear() - Pointer error");
                break;
            }
            delete[] matrix[i];
        }

        delete[] matrix;

        matrix = NULL;

        numRows = 0;
        numCols = 0;
        allocRows = 0;
    }



    bool SetMatrix(T value)
    {
        if (matrix == NULL)
            return false;

        for (unsigned int r = 0; r < numRows; ++r)
        {
            for (unsigned int c = 0; c < numCols; c++)
                Set(r, c, value);
        }
        return true;
    }


    bool ResetMatrix(void)
    {
        if (matrix == NULL)
            return false;

        for (unsigned int r = 0; r < numRows; ++r)
            memset((void*)matrix[r], 0, numCols * sizeof(T));

        return true;
    }


    //-- allocate memory, no initialization ---------------------------------------------
    BOOL Resize(UINT _numRows, UINT _numCols)
    {
#ifdef NO_MFC  //for min/max
        using namespace std;
#endif

        //-- remember original values 
        T** oldMatrix = matrix;
        UINT oldNumRows = numRows;
        UINT oldNumCols = numCols;

        numRows = _numRows;

        //-- if the new object has no rows, just keep the old matrix in place
        //-- NOTE: this is buggy if the number of columns changes....
        /*
        if ( numRows == 0 )
        {
        if ( numCols != _numCols )
        Report::ErrorMsg( "TMatrix::Resize() - this is buggy if the number of columns changes...." );

        numCols = _numCols;
        return TRUE;
        }
        */

        numCols = _numCols;

        matrix = new T * [numRows];
        allocRows = numRows;

        if (!matrix)
        {
            Report::ErrorMsg("TMatrix::Resize() - Insufficient Memory");
            return FALSE;
        }

        UINT i = 0;
        UINT j = 0;

        for (i = 0; i < numRows; i++)
        {
            matrix[i] = new T[numCols];
            memset(matrix[i], 0, sizeof(T) * numCols);
        }


        if (oldMatrix)
        {
            //-- copy original matrix into the new one
            UINT copyRows = min(numRows, oldNumRows);
            UINT copyCols = min(numCols, oldNumCols);

            for (i = 0; i < copyRows; i++)
            {
                for (j = 0; j < copyCols; j++)
                    Set(i, j, oldMatrix[i][j]);
            }

            //-- clean up old matrix allocations
            for (i = 0; i < oldNumRows; i++)
                delete[] oldMatrix[i];

            delete[] oldMatrix;
        }

        return TRUE;
    }

    //----------------------------------------------------------------------------------
    UINT AppendRow(T* data, int length)
    {
        if (numCols != length)
            return -1;

        //-- do we need to realloc the row array?
        if (allocRows <= numRows)
        {
            //-- reallocate matrix
            T** oldMatrix = matrix;

            allocRows += m_rowAllocIncr;
            matrix = new T * [allocRows];

            //-- copy previous row ptrs (if any)
            if (oldMatrix != NULL)
            {
                memcpy(matrix, oldMatrix, numRows * sizeof(T*));
                delete[] oldMatrix;
            }

            //-- NULL out unused ptrs
            for (UINT i = numRows; i < allocRows; i++)
                matrix[i] = NULL;
        }

        //-- reallocation done (if necessary), now allocate row
        matrix[numRows] = new T[numCols];

        if (matrix[numRows] == NULL)
        {
            Report::ErrorMsg("TMatrix::AppendRow() - Insufficient memory");
            return 0;
        }

        for (int i = 0; i < (int)numCols; i++)
            matrix[numRows][i] = data[i];

        //memcpy( matrix[ numRows ], data, numCols * sizeof( T ) );

        numRows++;
        return numRows;
    }


    //----------------------------------------------------------------------------------
    UINT AppendRows(UINT rowsToAdd)
    {
        //-- remember original values 
        T** oldMatrix = matrix;
        UINT oldNumRows = numRows;

        numRows += rowsToAdd;

        matrix = new T * [numRows];
        allocRows = numRows;

        if (!matrix)
        {
            Report::ErrorMsg("TMatrix::Resize() - Insufficient Memory");
            return FALSE;
        }

        UINT i = 0;

        for (i = 0; i < oldNumRows; i++)
            matrix[i] = oldMatrix[i];

        for (i = oldNumRows; i < numRows; i++)
        {
            matrix[i] = new T[numCols];
            memset(matrix[i], 0, sizeof(T) * numCols);
        }

        if (oldMatrix)
            delete[] oldMatrix;

        return numRows;
    }


    //----------------------------------------------------------------------------------
    UINT DeleteRow(UINT row)
    {
        ASSERT(row < numRows);

        // delete row
        delete[] matrix[row];

        // move everything from the deleted row to the end up in the matrix
        for (UINT i = row; i < numRows - 1; i++)
            matrix[i] = matrix[i + 1];

        numRows--;
        return numRows;
    }

    //----------------------------------------------------------------------------------
    UINT AppendCol(T* data, int count = 1)
    {
        Resize(numRows, numCols + count);  // add one more column

        // copy new data to col
        if (data != NULL)
        {
            for (int j = 0; j < count; j++)
                for (int i = 0; (UINT)i < numRows; i++)
                    Set(i, numCols - count + j, data[i]);
        }

        return numCols - 1;
    }

    //----------------------------------------------------------------------------------
    UINT InsertCol(int insertBefore, T* data)
    {
        if ((UINT)insertBefore >= numCols)
            return AppendCol(data);

        //-- remember original values 
        T** oldMatrix = matrix;
        numCols++;

        matrix = new T * [numRows];
        allocRows = numRows;

        if (!matrix)
        {
            Report::ErrorMsg("TMatrix::InsertCol() - Insufficient Memory");
            return (UINT)-1;
        }

        UINT i = 0;
        UINT j = 0;

        for (i = 0; i < numRows; i++)
        {
            matrix[i] = new T[numCols];
            memset(matrix[i], 0, sizeof(T) * numCols);
        }

        if (oldMatrix)
        {
            for (i = 0; i < numRows; i++)
            {
                for (j = 0; j < numCols; j++)
                {
                    if (j < (UINT)insertBefore)
                        Set(i, j, oldMatrix[i][j]);

                    else if (j == (UINT)insertBefore)
                        Set(i, j, data[j]);

                    else
                        Set(i, j, oldMatrix[i][j - 1]);
                }
            }

            //-- clean up old matrix allocations
            for (i = 0; i < numRows; i++)
                delete[] oldMatrix[i];

            delete[] oldMatrix;
        }

        return insertBefore;
    }


    //----------------------------------------------------------------------------------
    //-- get element ------------------------------------------------------------------
    T& operator () (UINT row, UINT col) { return Get(row, col); }

    //-- assignment -------------------------------------------------------------------
    TMatrix<T>& operator = (TMatrix<T>& /*t*/)
    {
        ASSERT(0);
        /*dummy body of function for dllexport explicitly instanciated--poison pill*/
        Clear();
        return *this;
    }

    //-- get/set a value ---------------------------------------------------------------
    T& Get(UINT row, UINT col)
    {
        if (row >= numRows)
        {
            CString msg;
            msg.Format("row out of bounds in TMatrix::Get() - Row %i, Row Count=%i", row, numRows);
            Report::ErrorMsg(msg);
            return matrix[numRows - 1][col];
        }

        else if (col >= numCols)
        {
            CString msg;
            msg.Format("col out of bounds in TMatrix::Get() - Col %i, Col Count=%i", col, numCols);
            Report::ErrorMsg(msg);
            return matrix[row][numCols - 1];
        }

        else
            return matrix[row][col];
    }


    //----------------------------------------------------------------------------------
    BOOL  Set(UINT row, UINT col, T value)
    {
        if (row >= numRows)
        {
            CString msg;
            msg.Format("row out of bounds in TMatrix::set() - Row %i, Row Count=%i", row, numRows);
            Report::ErrorMsg(msg);
            return FALSE;
        }

        else if (col >= numCols)
        {
            CString msg;
            msg.Format("col out of bounds in TMatrix::Set() - Col %i, Col Count=%i", col, numCols);
            Report::ErrorMsg(msg);
            return FALSE;
        }

        else
            matrix[row][col] = value;

        return TRUE;
    }

    //----------------------------------------------------------------------------------
    T** GetBase(void) { return matrix; }
    //----------------------------------------------------------------------------------
    T* GetRowPtr(UINT row) { return matrix[row]; }
};


