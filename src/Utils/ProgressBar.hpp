#pragma once

#include <atomic>
#include <mutex>

namespace VI
{
class ProgressBar final
{
public:
  ProgressBar(int total, int bar_width = 50);
  void Increment(int n = 1);
  void Finish();

private:
  int m_Total;
  int m_BarWidth;
  std::atomic<int> m_Current{0};
  std::atomic<int> m_LastPercent{-1};
  std::mutex m_Mutex;
};
} // namespace VI
