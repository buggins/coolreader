package org.coolreader.crengine;

import org.coolreader.CoolReader;
import org.coolreader.R;

import android.view.LayoutInflater;
import android.view.ViewGroup;
import android.widget.EditText;

public class BookInfoEditDialog extends BaseDialog {
	private CoolReader mActivity;
	private ReaderView mReaderView;
	private BookInfo mBookInfo;
	private LayoutInflater mInflater;
	public BookInfoEditDialog(CoolReader activity, ReaderView readerView, BookInfo book)
	{
		super(activity, null, false, false);
		this.mActivity = activity;
		this.mReaderView = readerView;
		this.mBookInfo = book;
	}

	@Override
	protected void onCreate() {
        setCancelable(true);
        setCanceledOnTouchOutside(true);

        mInflater = LayoutInflater.from(getContext());
        FileInfo file = mBookInfo.getFileInfo();
        ViewGroup view = (ViewGroup)mInflater.inflate(R.layout.book_info_edit_dialog, null);
        EditText edTitle = (EditText)view.findViewById(R.id.book_title);
        EditText edAuthor = (EditText)view.findViewById(R.id.book_author);
        EditText edSeriesName = (EditText)view.findViewById(R.id.book_series_name);
        EditText edSeriesNumber = (EditText)view.findViewById(R.id.book_series_number);
        edTitle.setText(file.title);
        edAuthor.setText(file.authors);
        edSeriesName.setText(file.series);
        edSeriesNumber.setText(file.seriesNumber);
        
        setView(view);
        super.onCreate();
		L.v("OptionsDialog is created");
	}
}
