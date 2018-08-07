package org.coolreader.crengine;

// By Muzikant@stackoverflow.com
// See https://stackoverflow.com/a/19272315 for details

public class ProcessWithTimeout extends Thread
{
    private Process m_process;
    private int m_exitCode = Integer.MIN_VALUE;

    public ProcessWithTimeout(Process p_process)
    {
        m_process = p_process;
    }

    public int waitForProcess(int p_timeoutMilliseconds)
    {
        this.start();

        try
        {
            this.join(p_timeoutMilliseconds);
        }
        catch (InterruptedException e)
        {
            this.interrupt();
        }

        return m_exitCode;
    }

    @Override
    public void run()
    {
        try
        {
            m_exitCode = m_process.waitFor();
        }
        catch (InterruptedException ignore)
        {
            // Do nothing
        }
        catch (Exception ex)
        {
            // Unexpected exception
        }
    }
}
