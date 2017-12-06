#include "src/mesh/Mesh.hpp"
#include <algorithm>
#include <numeric>
#include <unordered_map>
#include <utility>
#define _USE_MATH_DEFINES
#include <math.h>

using namespace grid;
using namespace point;
using namespace std;

Mesh::Mesh()
{
}
Mesh::~Mesh()
{
	for (auto& pt : pts)
		delete pt.int_reg;
}
void Mesh::process_geometry()
{
	setNebrId();
	set_geom_props();
	count_types();

	set_interaction_regions();
}
void Mesh::count_types()
{
	inner_size = border_size = frac_size = 0;
	Volume = 0.0;
	for (const auto& el : cells)
	{
		Volume += el.V;
		if (el.type == elem::HEX || 
			el.type == elem::BORDER_HEX || 
			el.type == elem::PRISM)							inner_size++;
		else if (el.type == elem::FRAC_QUAD)				frac_size++;
		else												border_size++;
	}
}
void Mesh::set_geom_props()
{
	double tmp1, tmp2;

	for (auto& el : cells)
	{
		if (el.type == elem::HEX || el.type == elem::BORDER_HEX)
		{
			// element center
			tmp1 = square(pts[el.verts[0]], pts[el.verts[1]], pts[el.verts[2]]);
			tmp2 = square(pts[el.verts[2]], pts[el.verts[3]], pts[el.verts[0]]);
			el.nebrs[0].cent = (tmp1 * (pts[el.verts[0]] + pts[el.verts[1]] + pts[el.verts[2]]) + tmp2 * (pts[el.verts[2]] + pts[el.verts[3]] + pts[el.verts[0]])) / (tmp1 + tmp2) / 3.0;
			tmp1 = square(pts[el.verts[4]], pts[el.verts[5]], pts[el.verts[6]]);
			tmp2 = square(pts[el.verts[6]], pts[el.verts[7]], pts[el.verts[4]]);
			el.nebrs[1].cent = (tmp1 * (pts[el.verts[4]] + pts[el.verts[5]] + pts[el.verts[6]]) + tmp2 * (pts[el.verts[6]] + pts[el.verts[7]] + pts[el.verts[4]])) / (tmp1 + tmp2) / 3.0;
			el.cent = (el.nebrs[0].cent + el.nebrs[1].cent) / 2.0;

			// bottom
			el.nebrs[0].S = square(pts[el.verts[0]], pts[el.verts[1]], pts[el.verts[2]], pts[el.verts[3]]);
			el.nebrs[0].L = distance(el.cent, el.nebrs[0].cent);
			// top
			el.nebrs[1].S = square(pts[el.verts[4]], pts[el.verts[5]], pts[el.verts[6]], pts[el.verts[7]]);
			el.nebrs[1].L = distance(el.cent, el.nebrs[1].cent);

			el.nebrs[2].S = square(pts[el.verts[0]], pts[el.verts[1]], pts[el.verts[5]], pts[el.verts[4]]);
			el.nebrs[2].cent = (pts[el.verts[0]] + pts[el.verts[1]] + pts[el.verts[5]] + pts[el.verts[4]]) / 4.0;
			el.nebrs[2].L = distance(el.cent, el.nebrs[2].cent);
			el.nebrs[2].n = vector_product(pts[el.verts[1]] - pts[el.verts[0]], pts[el.verts[4]] - pts[el.verts[0]]);
			el.nebrs[2].nu = { (el.nebrs[2].cent - el.cent).y, -(el.nebrs[2].cent - el.cent).x, 0.0 };
			el.T[0] = 2.0 * square(el.cent, el.nebrs[2].cent, el.nebrs[3].cent);

			el.nebrs[3].S = square(pts[el.verts[1]], pts[el.verts[2]], pts[el.verts[6]], pts[el.verts[5]]);
			el.nebrs[3].cent = (pts[el.verts[1]] + pts[el.verts[2]] + pts[el.verts[6]] + pts[el.verts[5]]) / 4.0;
			el.nebrs[3].L = distance(el.cent, el.nebrs[3].cent);
			el.nebrs[3].n = vector_product(pts[el.verts[2]] - pts[el.verts[1]], pts[el.verts[5]] - pts[el.verts[1]]);
			el.nebrs[3].nu = { (el.nebrs[3].cent - el.cent).y, -(el.nebrs[3].cent - el.cent).x, 0.0 };
			el.T[1] = 2.0 * square(el.cent, el.nebrs[3].cent, el.nebrs[4].cent);

			el.nebrs[4].S = square(pts[el.verts[2]], pts[el.verts[3]], pts[el.verts[7]], pts[el.verts[6]]);
			el.nebrs[4].cent = (pts[el.verts[2]] + pts[el.verts[3]] + pts[el.verts[7]] + pts[el.verts[6]]) / 4.0;
			el.nebrs[4].L = distance(el.cent, el.nebrs[4].cent);
			el.nebrs[4].n = vector_product(pts[el.verts[3]] - pts[el.verts[2]], pts[el.verts[6]] - pts[el.verts[2]]);
			el.nebrs[4].nu = { (el.nebrs[4].cent - el.cent).y, -(el.nebrs[4].cent - el.cent).x, 0.0 };
			el.T[2] = 2.0 * square(el.cent, el.nebrs[4].cent, el.nebrs[5].cent);

			el.nebrs[5].S = square(pts[el.verts[3]], pts[el.verts[0]], pts[el.verts[4]], pts[el.verts[7]]);
			el.nebrs[5].cent = (pts[el.verts[3]] + pts[el.verts[0]] + pts[el.verts[4]] + pts[el.verts[7]]) / 4.0;
			el.nebrs[5].L = distance(el.cent, el.nebrs[5].cent);
			el.nebrs[5].n = vector_product(pts[el.verts[0]] - pts[el.verts[3]], pts[el.verts[7]] - pts[el.verts[3]]);
			el.nebrs[5].nu = { (el.nebrs[5].cent - el.cent).y, -(el.nebrs[5].cent - el.cent).x, 0.0 };
			el.T[3] = 2.0 * square(el.cent, el.nebrs[5].cent, el.nebrs[2].cent);

			el.V = fabs(pts[el.verts[0]].z - pts[el.verts[4]].z) * (el.nebrs[0].S + el.nebrs[1].S) / 2.0;
		}
		else if (el.type == elem::PRISM)
		{
			el.cent = { 0, 0, 0 };
			for (int i = 0; i < el.verts_num; i++)
				el.cent += pts[el.verts[i]];
			el.cent /= el.verts_num;

			// top
			el.nebrs[0].S = square(pts[el.verts[0]], pts[el.verts[1]], pts[el.verts[2]]);
			el.nebrs[0].cent = (pts[el.verts[0]] + pts[el.verts[1]] + pts[el.verts[2]]) / 3.0;
			el.nebrs[0].L = distance(el.cent, el.nebrs[0].cent);
			// bottom
			el.nebrs[1].S = square(pts[el.verts[3]], pts[el.verts[4]], pts[el.verts[5]]);
			el.nebrs[1].cent = (pts[el.verts[3]] + pts[el.verts[4]] + pts[el.verts[5]]) / 3.0;
			el.nebrs[1].L = distance(el.cent, el.nebrs[1].cent);

			el.nebrs[2].S = square(pts[el.verts[0]], pts[el.verts[1]], pts[el.verts[4]], pts[el.verts[3]]);
			el.nebrs[2].cent = (pts[el.verts[0]] + pts[el.verts[1]] + pts[el.verts[4]] + pts[el.verts[3]]) / 4.0;
			el.nebrs[2].L = distance(el.cent, el.nebrs[2].cent);
			el.nebrs[2].n = vector_product(pts[el.verts[1]] - pts[el.verts[0]], pts[el.verts[3]] - pts[el.verts[0]]) / 2.0;
			el.nebrs[2].nu = { (el.nebrs[2].cent - el.cent).y, -(el.nebrs[2].cent - el.cent).x, 0.0 };
			el.T[0] = 2.0 * square(el.cent, el.nebrs[2].cent, el.nebrs[3].cent);

			el.nebrs[3].S = square(pts[el.verts[1]], pts[el.verts[2]], pts[el.verts[5]], pts[el.verts[4]]);
			el.nebrs[3].cent = (pts[el.verts[1]] + pts[el.verts[2]] + pts[el.verts[5]] + pts[el.verts[4]]) / 4.0;
			el.nebrs[3].L = distance(el.cent, el.nebrs[3].cent);
			el.nebrs[3].n = vector_product(pts[el.verts[2]] - pts[el.verts[1]], pts[el.verts[4]] - pts[el.verts[1]]) / 2.0;
			el.nebrs[3].nu = { (el.nebrs[3].cent - el.cent).y, -(el.nebrs[3].cent - el.cent).x, 0.0 };
			el.T[1] = 2.0 * square(el.cent, el.nebrs[3].cent, el.nebrs[4].cent);

			el.nebrs[4].S = square(pts[el.verts[2]], pts[el.verts[0]], pts[el.verts[3]], pts[el.verts[5]]);
			el.nebrs[4].cent = (pts[el.verts[2]] + pts[el.verts[0]] + pts[el.verts[3]] + pts[el.verts[5]]) / 4.0;
			el.nebrs[4].L = distance(el.cent, el.nebrs[4].cent);
			el.nebrs[4].n = vector_product(pts[el.verts[0]] - pts[el.verts[2]], pts[el.verts[5]] - pts[el.verts[2]]) / 2.0;
			el.nebrs[4].nu = { (el.nebrs[4].cent - el.cent).y, -(el.nebrs[4].cent - el.cent).x, 0.0 };
			el.T[2] = 2.0 * square(el.cent, el.nebrs[4].cent, el.nebrs[2].cent);

			el.V = fabs(pts[el.verts[0]].z - pts[el.verts[3]].z) * (el.nebrs[0].S + el.nebrs[1].S) / 2.0;
		}
		else if (el.type == elem::BORDER_TRI)
		{
			el.nebrs[0].S = square(pts[el.verts[0]], pts[el.verts[1]], pts[el.verts[2]]);
			el.cent = el.nebrs[0].cent = (pts[el.verts[0]] + pts[el.verts[1]] + pts[el.verts[2]]) / 3.0;
			el.nebrs[0].L = el.V = 0.0;
		}
		else if (el.type == elem::BORDER_QUAD)
		{
			el.nebrs[0].S = square(pts[el.verts[0]], pts[el.verts[1]], pts[el.verts[2]], pts[el.verts[3]]);
			tmp1 = square(pts[el.verts[0]], pts[el.verts[1]], pts[el.verts[2]]);
			tmp2 = square(pts[el.verts[2]], pts[el.verts[3]], pts[el.verts[0]]);
			el.cent  = el.nebrs[0].cent = (tmp1 * (pts[el.verts[0]] + pts[el.verts[1]] + pts[el.verts[2]]) + tmp2 * (pts[el.verts[2]] + pts[el.verts[3]] + pts[el.verts[0]])) / (tmp1 + tmp2) / 3.0;
			el.nebrs[0].L = el.V = 0.0;
			el.nebrs[0].n = vector_product(pts[el.verts[1]] - pts[el.verts[0]], pts[el.verts[3]] - pts[el.verts[0]]);
		}
		else if (el.type == elem::FRAC_QUAD)
		{
			el.nebrs[0].S = el.nebrs[1].S = square(pts[el.verts[0]], pts[el.verts[1]], pts[el.verts[2]], pts[el.verts[3]]);
			el.cent = el.nebrs[0].cent = el.nebrs[1].cent = (pts[el.verts[0]] + pts[el.verts[1]] + pts[el.verts[2]] + pts[el.verts[3]]) / 4.0;
			el.nebrs[0].L = el.nebrs[1].L = el.V = 0.0;
			el.nebrs[0].n = vector_product(pts[el.verts[1]] - pts[el.verts[0]], pts[el.verts[3]] - pts[el.verts[0]]);
		}
	}
};
void Mesh::set_interaction_regions()
{
	// Getting max z-axis coordinate
	const double z_max = max_element(pts.begin(), pts.end(), [&](const Point& pt1, const Point& pt2) {return pt1.z < pt2.z; })->z;
	
	bool isBorder = true;
	for (auto& pt : pts)
	{
		isBorder = true;
		// Check is not border
		for (const auto& cell : pt.cells)
			if (cells[cell].type != elem::BORDER_HEX)
			{
				isBorder = false;
				break;
			}
		// All except top z-plane
		if (!isBorder && fabs(pt.z - z_max) > EQUALITY_TOLERANCE)
		{
			pt.int_reg = new Interaction(pt.cells);
			auto& cur_cells = pt.int_reg->cells;
			cur_cells.erase(remove_if(cur_cells.begin(), cur_cells.end(), [&](Id cell_id) { return (cells[cell_id.cell].cent.z - pt.z < 0.0) ? true : false; }), cur_cells.end());
		}
	}
	
	// Order the cells in interaction regions
	for (auto& pt : pts)
	{
		if (pt.int_reg != nullptr)
		{
			auto& ireg_cells = pt.int_reg->cells;
			sort(ireg_cells.begin(), ireg_cells.end(), [=](Id& id1, Id& id2)
			{
				double phi1, phi2;
				const Cell& cell1 = cells[id1.cell];	const Cell& cell2 = cells[id2.cell];
				const double x1 = cell1.cent.x - pt.x;	const double y1 = cell1.cent.y - pt.y;
				const double x2 = cell2.cent.x - pt.x;	const double y2 = cell2.cent.y - pt.y;
				if (x1 < 0.0)
					phi1 = M_PI + atan(y1 / x1);
				else
					phi1 = (y1 < 0.0 ? 2.0 * M_PI + atan(y1 / x1) : atan(y1 / x1));
				if (x2 < 0.0)
					phi2 = M_PI + atan(y2 / x2);
				else
					phi2 = (y2 < 0.0 ? 2.0 * M_PI + atan(y2 / x2) : atan(y2 / x2));

				return phi1 < phi2;
			});
		}
	}

	// Fill pointers to interaction regions in cells neighbours
	for (int i = 0; i < inner_size; i++)
	{
		Cell& cell = cells[i];
		if (cell.type == elem::HEX || cell.type == elem::BORDER_HEX)
		{
			// 1 or 5
			const auto& cells1_plus = pts[cell.verts[1]].int_reg->cells;
			cell.nebrs[2].ireg_plus = (find(cells1_plus.begin(), cells1_plus.end(), cell.id) != cells1_plus.end() ?
				pts[cell.verts[1]].int_reg : pts[cell.verts[5]].int_reg);
			// 0 or 4
			const auto& cells1_minus = pts[cell.verts[0]].int_reg->cells;
			cell.nebrs[2].ireg_minus = (find(cells1_minus.begin(), cells1_minus.end(), cell.id) != cells1_minus.end() ?
				pts[cell.verts[0]].int_reg : pts[cell.verts[4]].int_reg);

			// 2 or 6
			const auto& cells2_plus = pts[cell.verts[2]].int_reg->cells;
			cell.nebrs[3].ireg_plus = (find(cells2_plus.begin(), cells2_plus.end(), cell.id) != cells2_plus.end() ?
				pts[cell.verts[2]].int_reg : pts[cell.verts[6]].int_reg);
			// 1 or 5
			cell.nebrs[3].ireg_minus = cell.nebrs[2].ireg_plus;

			// 3 or 7
			const auto& cells3_plus = pts[cell.verts[3]].int_reg->cells;
			cell.nebrs[4].ireg_plus = (find(cells3_plus.begin(), cells3_plus.end(), cell.id) != cells3_plus.end() ?
										pts[cell.verts[3]].int_reg : pts[cell.verts[7]].int_reg);
			// 2 or 6
			cell.nebrs[4].ireg_minus = cell.nebrs[3].ireg_plus;

			// 0 or 4
			cell.nebrs[5].ireg_plus = cell.nebrs[2].ireg_minus;
			// 3 or 3
			cell.nebrs[5].ireg_minus = cell.nebrs[4].ireg_plus;
		}
		else if (cell.type == elem::PRISM)
		{
			// 1 or 4
			const auto& cells1_plus = pts[cell.verts[1]].int_reg->cells;
			cell.nebrs[2].ireg_plus = (find(cells1_plus.begin(), cells1_plus.end(), cell.id) != cells1_plus.end() ?
										pts[cell.verts[1]].int_reg : pts[cell.verts[4]].int_reg);
			// 0 or 3
			const auto& cells1_minus = pts[cell.verts[0]].int_reg->cells;
			cell.nebrs[2].ireg_minus = (find(cells1_minus.begin(), cells1_minus.end(), cell.id) != cells1_minus.end() ?
										pts[cell.verts[0]].int_reg : pts[cell.verts[3]].int_reg);
			
			// 2 or 5
			const auto& cells2_plus = pts[cell.verts[2]].int_reg->cells;
			cell.nebrs[3].ireg_plus = (find(cells2_plus.begin(), cells2_plus.end(), cell.id) != cells2_plus.end() ?
										pts[cell.verts[2]].int_reg : pts[cell.verts[5]].int_reg);
			// 1 or 4
			cell.nebrs[3].ireg_minus = cell.nebrs[2].ireg_plus;

			// 0 or 3
			cell.nebrs[4].ireg_plus = cell.nebrs[2].ireg_minus;
			// 2 or 5
			cell.nebrs[4].ireg_minus = cell.nebrs[3].ireg_plus;
		}	

		// Fill nebr indices in interaction regions
		/*for (char i = 0; i < cell.nebrs_num; i++)
		{
			auto& reg_cells = cell.nebrs[i].ireg_minus->cells;
			auto& it = find(reg_cells.begin(), reg_cells.end(), cell.id);
			if (it != reg_cells.end())
			{
				it->nebr = i;
			}
		}*/
	}
}
size_t Mesh::getCellsSize() const
{
	return cells.size();
}
void Mesh::setNebrId()
{
	for (auto& el : cells)
		for (char i = 0; i < el.nebrs_num; i++)
			el.nebrs[i].nebr.nebr = findNebrId(el, cells[el.nebrs[i].nebr.cell]);
}