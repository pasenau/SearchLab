#pragma once

#include <array>	
#include <vector>
#include <limits>
#include <cmath>

#include "bins_cells_container.h"
#include "spatial_search_result.h"




template <typename TObjectType>
class PointsBins {
	static constexpr int Dimension = 3;
public:
	using InternalPointType = std::array<double, Dimension>;
	using PointerType = TObjectType*;
	using ResultType = SpatialSearchResult<TObjectType>;

	template<typename TIteratorType>
	PointsBins(TIteratorType const& PointsBegin, TIteratorType const& PointsEnd) : mCells(PointsBegin, PointsEnd) {
		mNumberOfPoints = std::distance(PointsBegin, PointsEnd);
		if (mNumberOfPoints == 0) {
			mpPoints = nullptr;
			return;
		}
		mpPoints = new PointerType[mNumberOfPoints];
		for (std::size_t i = 0; i < mNumberOfPoints; i++)
			mpPoints[i] = nullptr;

		AssignPointsToCells(PointsBegin, PointsEnd);
	}

	void SearchInRadius(TObjectType const& ThePoint, double Radius, std::vector<ResultType>& rResults) {
		InternalPointType min_point;
		std::array<std::size_t, Dimension> length;
		const double radius2 = Radius * Radius;

		for (int i = 0; i < Dimension; i++) {
			min_point[i] = ThePoint[i] - Radius;
			length[i] = mCells.CalculatePosition(ThePoint[i] + Radius, i) - mCells.CalculatePosition(ThePoint[i] - Radius, i) + 1;
		}
		auto min_cell = mCells.CalculateCellIndex(min_point);

		for (std::size_t i_z = 0; i_z < length[2]; i_z++) {
			auto y_position = min_cell + i_z * mCells.GetNumberOfCells(0) *mCells.GetNumberOfCells(1);
			for (std::size_t i_y = 0; i_y < length[1]; i_y++) {
				std::size_t offset = mCells.GetCellBeginIndex(y_position);
				const std::size_t end_offset = mCells.GetCellBeginIndex(y_position + length[0]);
				TObjectType** p_point = mpPoints + mCells.GetCellBeginIndex(y_position);
				for (; offset <end_offset; offset++) {
					if (Distance2(**p_point, ThePoint) <= radius2) {
						rResults.push_back(ResultType(*p_point));
					}
					p_point++;
				}
				y_position += mCells.GetNumberOfCells(0);
			}
		}
	}

	ResultType SearchNearest(TObjectType const& ThePoint) {
		auto cell_index = mCells.CalculateCellIndex(ThePoint);
		ResultType current_result;

		if (mNumberOfPoints == 0)
			return current_result;

		current_result.SetDistance2(std::numeric_limits<double>::max());
		double radius = std::max(mCells.GetCellSize(0), mCells.GetCellSize(1));
		radius = std::max(radius, mCells.GetCellSize(2)) * .5;

		while (!current_result.IsObjectFound()) {
			InternalPointType min_point;
			std::array<std::size_t, Dimension> length;
			const double radius2 = radius * radius;

			for (int i = 0; i < Dimension; i++) {
				min_point[i] = ThePoint[i] - radius;
				length[i] = mCells.CalculatePosition(ThePoint[i] + radius, i) - mCells.CalculatePosition(ThePoint[i] - radius, i) + 1;
			}
			auto min_cell = mCells.CalculateCellIndex(min_point);

			for (std::size_t i_z = 0; i_z < length[2]; i_z++) {
				auto y_position = min_cell + i_z * mCells.GetNumberOfCells(0) * mCells.GetNumberOfCells(1);
				for (std::size_t i_y = 0; i_y < length[1]; i_y++) {
					std::size_t offset = mCells.GetCellBeginIndex(y_position);
					const std::size_t end_offset = mCells.GetCellBeginIndex(y_position + length[0]);
					TObjectType** p_point = mpPoints + mCells.GetCellBeginIndex(y_position);
					for (; offset <end_offset; offset++) {
						double distance_2 = Distance2(**p_point, ThePoint);
						if (distance_2 < current_result.GetDistance2()) {
							current_result.Set(*p_point);
							current_result.SetDistance2(distance_2);
						}
						p_point++;
					}
					y_position += mCells.GetNumberOfCells(0);
				}
			}
			radius *= 2.00;
		}
		return current_result;
	}


private:
	std::size_t mNumberOfPoints;
	BinsCellsContainer mCells;
	TObjectType** mpPoints;

	template<typename TIteratorType>
	void AssignPointsToCells(TIteratorType const& PointsBegin, TIteratorType const& PointsEnd) {
		for (auto i_point = PointsBegin; i_point != PointsEnd; i_point++) {
			auto index = mCells.CalculateCellIndex(*i_point);
			for (std::size_t offset = mCells.GetCellBeginIndex(index); offset < mCells.GetCellBeginIndex(index + 1); offset++)
				if (mpPoints[offset] == nullptr) {
					mpPoints[offset] = &(*i_point);
					break;
				}
		}

	}

	void SearchNearestInCell(std::size_t CellIndex, TObjectType const& ThePoint, ResultType& rCurrentResult) {
		for (std::size_t offset = mCells.GetCellBeginIndex(CellIndex); offset <mCells.GetCellBeginIndex(CellIndex + 1); offset++) {
			TObjectType* p_point = mpPoints[offset];
			auto distance_2 = Distance2(*p_point, ThePoint);
			if ( distance_2 <= rCurrentResult.GetDistance2()) {
				rCurrentResult.Set(p_point);
				rCurrentResult.SetDistance2(distance_2);
			}
		}
	}

	double Distance2(TObjectType const& FirstPoint, TObjectType const& SecondPoint) {
		double result = double();
		for (int i = 0; i < Dimension; i++) {
			auto distance_i = FirstPoint[i] - SecondPoint[i];
			result += distance_i * distance_i;
		}
		return result;
	}
};