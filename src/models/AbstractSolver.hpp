#ifndef ABSTRACTSOLVER_HPP_
#define ABSTRACTSOLVER_HPP_

#include <algorithm>
#include <cmath>
#include <iostream>
#include <array>
#include <vector>

template <class modelType>
class AbstractSolver {
public:
	typedef modelType Model;
	typedef typename Model::Mesh Mesh;
	typedef typename Model::Cell Cell;
	static const int var_size = Model::var_size;
protected:
	double** jac;
	double* y;

	Model* model;
	Mesh* mesh;
	const size_t size;
			
	int curTimePeriod;
	const double Tt;
	double cur_t, cur_t_log;

	int idx1, idx2;

	double t_dim;

	int iterations;
		
	void copyIterLayer();
	void revertIterLayer();
	void copyTimeLayer();
		
	double convergance(int& ind, int& varInd);
	double averValue(int varInd);
	void averValue(std::array<double, var_size>& aver);
		
	virtual void writeData() = 0;
	virtual void control() = 0;
	virtual void doNextStep();
	virtual void solveStep() = 0;
	double NEWTON_STEP;
	double CHOP_MULT;
	double MAX_SAT_CHANGE;
	double CONV_W2, CONV_VAR;
	int MAX_ITER;

	virtual void checkStability();

	std::vector<int> stencil_idx;
	inline void getMatrixStencil(const Cell& cell)
	{
		if (cell.type == elem::BORDER_HEX || cell.type == elem::FRAC_HEX)
		{
			stencil_idx.resize(1);
			stencil_idx[0] = cell.id;
		}
		else
		{
			std::vector<int> tmp;
			for (int i = 2; i < cell.nebrs_num; i++)
				for (const auto& id : cell.nebrs[i].ireg[point::PLUS]->cells)
					tmp.push_back(id.cell);
			sort(tmp.begin(), tmp.end());
			tmp.erase(std::unique(tmp.begin(), tmp.end()), tmp.end());

			stencil_idx.resize(2 + tmp.size());
			stencil_idx[0] = cell.id;
			stencil_idx[1] = cell.nebrs[0].nebr.cell;
			stencil_idx[2] = cell.nebrs[1].nebr.cell;
			int counter = 3;
			for (int i = 0; i < tmp.size(); i++)
				if (tmp[i] != cell.id)
					stencil_idx[counter++] = tmp[i];
		}
	};

	int* ind_i;
	int* ind_j;
	double* a;
	int* ind_rhs;
	double* rhs;
	int* cols;
	// Number of non-zero elements in sparse matrix
	int elemNum;

	int options[4];
	int repeat;
public:
	AbstractSolver(modelType* _model);
	virtual ~AbstractSolver();
		
	virtual void fill();
	void fillIndices()
	{
		int counter = 0;

		for (int i = 0; i < model->varNum; i++)
		{
			const auto& cell = mesh->cells[i];
			getMatrixStencil(cell);
			for (size_t i = 0; i < var_size; i++)
				for (const int idx : stencil_idx)
					for (size_t j = 0; j < var_size; j++)
					{
						ind_i[counter] = var_size * cell.id + i;			ind_j[counter++] = var_size * idx + j;
					}
			stencil_idx.clear();
		}

		elemNum = counter;
		
		for (int i = 0; i < var_size * model->cellsNum; i++)
			ind_rhs[i] = i;
	}
	virtual void start();
};

#endif /* ABSTRACTSOLVER_HPP_ */
