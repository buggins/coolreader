package org.coolreader.plugins;

public class AsyncOperationControl {
	boolean finished;
	boolean cancelled;
	public void cancel() {
		cancelled = true;
	}
	public boolean isCancelled() {
		return cancelled;
	}
	public void finished() {
		finished = true;
	}
	public boolean isFinished() {
		return finished;
	}
	public boolean isActive() {
		return !finished && !cancelled;
	}
}
