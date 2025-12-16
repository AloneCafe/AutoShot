#pragma once
#include "pch.h"

#include <chrono>
#include <functional>
#include <thread>
#include <atomic>
#include <stdexcept>
#include <iostream>

// 高精度定时器类，支持单次/周期性回调
class FHighResTimer {
public:
	using Clock = std::chrono::high_resolution_clock;
	using TimePoint = std::chrono::time_point<Clock>;
	using Duration = std::chrono::milliseconds;  // 内部统一使用精度
	using Callback = std::function<void()>;

	// 构造函数：interval为间隔，loop是否循环，cb回调函数
	FHighResTimer(Duration interval, bool loop, Callback cb);

	FHighResTimer();

	~FHighResTimer();

	// 启动定时器
	void start();

	// 停止定时器（阻塞等待线程退出）
	void stop();

	// 判断定时器是否运行中
	bool is_running() const;

	void set_duration(Duration interval) {
		interval_ = interval;
	}

	Duration get_duration() {
		return interval_;
	}

	void set_callback(Callback cb) {
		callback_ = cb;
	}

	void set_loop(bool loop) {
		loop_ = loop;
	}

private:
	// 工作线程主循环
	void run();

	Duration interval_;       // 定时间隔
	bool loop_;               // 是否周期性执行
	Callback callback_;       // 回调函数
	std::atomic<bool> running_;     // 线程运行标志
	std::unique_ptr<std::thread> thread_;  // 工作线程

public:
	std::atomic<bool> externalTerminateFlag = false;
	std::atomic<bool> m_stopped = true;
};

