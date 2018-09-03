package com.onyx.android.sdk.data;

public class RefValue<T>
{
    private T mValue = null;
    
    public RefValue()
    {
    }
    
    public RefValue(T v)
    {
        mValue = v;
    }
    
    public T getValue()
    {
        return mValue;
    }
    public void setValue(T v)
    {
        mValue = v;
    }
}
