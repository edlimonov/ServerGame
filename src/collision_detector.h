#pragma once

#include "geom.h"

#include <algorithm>
#include <vector>

#include <type_traits>
#include <concepts>
#include <typeinfo>

namespace collision_detector {

struct CollectionResult {
    bool IsCollected(double collect_radius) const {
        return proj_ratio >= 0 && proj_ratio <= 1 && sq_distance <= collect_radius * collect_radius;
    }

    // квадрат расстояния до точки
    double sq_distance;

    // доля пройденного отрезка
    double proj_ratio;
};

// Движемся из точки a в точку b и пытаемся подобрать точку c.
// Эта функция реализована в уроке.
CollectionResult TryCollectPoint(geom::Point2D a, geom::Point2D b, geom::Point2D c);

struct Item {
    geom::Point2D position;
    double width;
};

struct Gatherer {
    geom::Point2D start_pos;
    geom::Point2D end_pos;
    double width;
};

class ItemGathererProvider {
protected:
    ~ItemGathererProvider() = default;

public:
    virtual size_t ItemsCount() const = 0;
    virtual Item GetItem(size_t idx) const = 0;
    virtual size_t GatherersCount() const = 0;
    virtual Gatherer GetGatherer(size_t idx) const = 0;
};

class ItemGatherer : public collision_detector::ItemGathererProvider {
 public:
  //Methods
  size_t ItemsCount() const override {
    return items_.size();
  }

  size_t GatherersCount() const override {
    return gatherers_.size();
  }

  collision_detector::Item GetItem(size_t pos) const override {
    return items_.at(pos);
  }

  collision_detector::Gatherer GetGatherer(size_t pos) const override {
    return gatherers_.at(pos);
  }

  void AddItem(const collision_detector::Item &item) {
    items_.push_back(item);
  }

  void AddGatherer(const collision_detector::Gatherer &gatherer) {
    gatherers_.push_back(gatherer);
  }

 private:
  std::vector<collision_detector::Item> items_;
  std::vector<collision_detector::Gatherer> gatherers_;
};

struct GatheringEvent {
    size_t item_id;
    size_t gatherer_id;
    double sq_distance;
    double time;
};

std::vector<GatheringEvent> FindGatherEvents(const ItemGathererProvider& provider);

}  // namespace collision_detector
