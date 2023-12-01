#ifndef V2X_ETSI_ASN1_LIB_UTILS_HPP
#define V2X_ETSI_ASN1_LIB_UTILS_HPP

#include "v2x_etsi_asn1_lib/units.h"
#include <AngularSpeedConfidence.h>
#include <YawRateConfidence.h>
#include <Wgs84Angle.h>
#include <cmath>

static const double confidence95 = 1.96;

static inline auto encodeValue(auto value, double unit, auto oorL, auto oorH)
{
  return std::clamp<int64_t>(
      static_cast<int64_t>(std::round(value / unit)), static_cast<int64_t>(oorL), static_cast<int64_t>(oorH));
}

static inline auto encodeConfidenceFromStdDev(double stddev, double unit, auto oor)
{
  return std::clamp<int64_t>(
      static_cast<int64_t>(std::round(stddev / unit * confidence95)), 0, static_cast<int64_t>(oor));
}

static inline auto encodeWgsAngleNoCorrection(double angle_rad)
{
  double conf_heading =
      ((static_cast<double>(Wgs84AngleValue_wgs84East) * Wgs84AngleValueUnit_degrees) - (angle_rad * 180. / M_PI));
  conf_heading = std::fmod(conf_heading, 360);
  if (conf_heading < 0)
  {
    conf_heading += 360;
  }
  return encodeValue(conf_heading, Wgs84AngleValueUnit_degrees, Wgs84AngleValue_wgs84North, Wgs84AngleValue_doNotUse);
}

static inline auto decodeWgsAngleNoCorrection(Wgs84AngleValue_t value)
{
  // add offset to 0 being East, and invert the angle because it is defined in the opposite direction
  auto yaw = (-(static_cast<double>(value - Wgs84AngleValue_wgs84East)) * Wgs84AngleValueUnit_degrees) / 180. * M_PI;
  return yaw;
}

static inline auto decodeConfidenceAngle(auto conf, double unit)
{
  return std::pow(static_cast<double>(conf) * unit / confidence95 * M_PI / 180., 2);
}

static inline auto decodeConfidence(auto conf, double unit)
{
  return std::pow(static_cast<double>(conf) * unit / confidence95, 2);
}

template <typename T>
static inline T* alloc_asn1()
{
  auto* ptr = static_cast<T*>(calloc(1, sizeof(T)));
  assert(ptr && "calloc() failed");
  return ptr;
}

static const std::map<double, AngularSpeedConfidence_t> deg_conf_2_angular_speed_conf = {
  { 1.0, AngularSpeedConfidence_degSec_01 },  { 2.0, AngularSpeedConfidence_degSec_02 },
  { 5.0, AngularSpeedConfidence_degSec_05 },  { 10.0, AngularSpeedConfidence_degSec_10 },
  { 20.0, AngularSpeedConfidence_degSec_20 }, { 50.0, AngularSpeedConfidence_degSec_50 },
};

static const std::map<double, YawRateConfidence_t> deg_conf_2_yawrate_conf = {
  { 0.01, YawRateConfidence_degSec_000_01 }, { 0.05, YawRateConfidence_degSec_000_05 },
  { 0.1, YawRateConfidence_degSec_000_10 },  { 1., YawRateConfidence_degSec_001_00 },
  { 5., YawRateConfidence_degSec_005_00 },   { 10., YawRateConfidence_degSec_010_00 },
  { 100., YawRateConfidence_degSec_100_00 }
};

// Given a map from keys to values, creates a new map from values to keys
template <typename K, typename V>
static inline std::map<V, K> reverse_map(const std::map<K, V>& m)
{
  std::map<V, K> r;
  for (const auto& kv : m)
  {
    r[kv.second] = kv.first;
  }
  return r;
}

static const auto angular_speed_conf_2_deg_conf = reverse_map(deg_conf_2_angular_speed_conf);
static const auto yawrate_conf_2_deg_conf = reverse_map(deg_conf_2_yawrate_conf);

static inline AngularSpeedConfidence_t get_angular_speed_confidence(double stddev)
{
  auto conf = stddev * confidence95 * 180.0 / M_PI;
  for (const auto& e : deg_conf_2_angular_speed_conf)
  {
    if (conf < e.first)
    {
      return e.second;
    }
  }
  return AngularSpeedConfidence_outOfRange;
}

static inline double get_angular_speed_confidence_inverse(AngularSpeedConfidence_t confidence)
{
  double mult = confidence95 * 180.0 / M_PI;
  auto res = angular_speed_conf_2_deg_conf.find(confidence);
  if (res != angular_speed_conf_2_deg_conf.end())
  {
    return res->second / mult;
  }
  return 50 / mult;
}

static inline YawRateConfidence_t get_yawrate_confidence(double stddev)
{
  auto conf = stddev * confidence95 * 180.0 / M_PI;
  for (const auto& e : deg_conf_2_yawrate_conf)
  {
    if (conf < e.first)
    {
      return e.second;
    }
  }
  return YawRateConfidence_outOfRange;
}

static inline double get_yawrate_confidence_inverse(YawRateConfidence_t confidence)
{
  double mult = confidence95 * 180.0 / M_PI;
  auto res = yawrate_conf_2_deg_conf.find(confidence);
  if (res != yawrate_conf_2_deg_conf.end())
  {
    return res->second / mult;
  }
  return 100 / mult;
}

#endif  // V2X_ETSI_ASN1_LIB_UTILS_HPP
