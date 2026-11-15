#pragma once
namespace logisth {
class ImpulseProcessor {
  public:
    virtual ~ImpulseProcessor() = default;
    virtual void process(const float *left, const float *right, float *outLeft, float *outRight,
                         int count, bool normalize) noexcept = 0;
    virtual void reset() noexcept = 0;
};
} // namespace logisth
