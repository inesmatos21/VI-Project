#include "ProgressBar.hpp"

#include <iomanip>
#include <iostream>
#include <mutex>
#include <ostream>

namespace VI
{

ProgressBar::ProgressBar(int total, int bar_width) : m_Total{total}, m_BarWidth{bar_width}, m_Current{0}, m_LastPercent{} {}

void ProgressBar::Increment(int n)
{
  int done = m_Current.fetch_add(n) + n;
  int percent = (100 * done) / m_Total;

  if (percent != m_LastPercent.load())
  {
    std::lock_guard<std::mutex> lock{m_Mutex};

    if (percent != m_LastPercent.load())
    { // double-check after lock
      m_LastPercent = percent;
      std::cout << "\rRendering: [";
      int pos = m_BarWidth * percent / 100;

      for (int i = 0; i < m_BarWidth; ++i)
      {
        std::cout << (i < pos ? "=" : (i == pos ? ">" : " "));
      }
      std::cout << "] " << std::setw(3) << percent << "%";
      std::cout.flush();
    }
  }
}

void ProgressBar::Finish()
{
  std::cout << std::endl;
}

} // namespace VI
