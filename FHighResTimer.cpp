#include "pch.h"
#include "FHighResTimer.h"

FHighResTimer::FHighResTimer(Duration interval, bool loop, Callback cb) : interval_(interval), loop_(loop), callback_(std::move(cb)),
running_(false), thread_(nullptr)
{
	if (interval <= Duration::zero()) {
		throw std::invalid_argument("Interval must be positive");
	}
	if (!callback_) {
		throw std::invalid_argument("Callback cannot be null");
	}
}

FHighResTimer::FHighResTimer()
{
}

FHighResTimer::~FHighResTimer()
{
	stop();  // 析构时确保线程停止
}

void FHighResTimer::start()
{
	if (running_.exchange(true)) {
		throw std::runtime_error("Timer already running");
	}
	// 启动工作线程，绑定run()方法
	thread_ = std::make_unique<std::thread>(&FHighResTimer::run, this);
}

void FHighResTimer::stop()
{
	if (m_stopped.load()) {
		return;
	}
	if (running_.exchange(false)) {
		if (thread_ && thread_->joinable()) {
			thread_->join();  // 等待线程安全退出
			thread_.reset();
			m_stopped.store(true);
		}
	}
}

bool FHighResTimer::is_running() const
{
	return running_.load();
}

void FHighResTimer::run()
{
	m_stopped = false;
	TimePoint next_trigger = Clock::now() + interval_;  // 首次触发时间

	while (running_.load()) {
		// 粗等待：休眠至接近触发时间（预留100us自旋窗口）
		const auto spin_window = std::chrono::microseconds(100);
		if (next_trigger > Clock::now() + spin_window) {
			std::this_thread::sleep_until(next_trigger - spin_window);
		}

		// 自旋等待：精确到触发时刻
		while (Clock::now() < next_trigger && running_.load()) {
			std::this_thread::yield();  // 让出CPU，减少忙等开销
		}

		if (!running_.load()) break;

		if (externalTerminateFlag) {
			
			break;
		}

		// 执行回调（捕获异常避免线程崩溃）
		try {
			callback_();
		}
		catch (const std::exception& e) {
			std::cerr << "Timer callback error: " << e.what() << std::endl;
		}

		// 非循环模式下执行一次后退出
		if (!loop_) break;

		// 计算下次触发时间（绝对时间，避免累积误差）
		next_trigger += interval_;
		// 防止系统时间回拨导致的定时器积压
		if (next_trigger < Clock::now()) {
			next_trigger = Clock::now() + interval_;
		}
	}
	m_stopped.store(true);
	running_.store(false);
}
