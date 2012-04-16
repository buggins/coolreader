package org.coolreader.crengine;

import java.util.ArrayList;

import org.coolreader.CoolReader;
import org.coolreader.R;
import org.coolreader.crengine.CoverpageManager.CoverpageBitmapReadyListener;

import android.graphics.Bitmap;
import android.graphics.Bitmap.Config;
import android.graphics.drawable.BitmapDrawable;
import android.os.Bundle;
import android.text.Editable;
import android.text.TextWatcher;
import android.view.LayoutInflater;
import android.view.ViewGroup;
import android.widget.EditText;
import android.widget.ImageView;
import android.widget.LinearLayout;
import android.widget.RadioGroup;
import android.widget.RatingBar;

public class BookInfoEditDialog extends BaseDialog {
	private CoolReader mActivity;
	private ReaderView mReaderView;
	private BookInfo mBookInfo;
	private LayoutInflater mInflater;
	private int mWindowSize;
	public BookInfoEditDialog(CoolReader activity, ReaderView readerView, BookInfo book, int windowSize)
	{
		super(activity, null, false, false);
		this.mWindowSize = windowSize;
		this.mActivity = activity;
		this.mReaderView = readerView;
		this.mBookInfo = book;
	}

	@Override
	protected void onCreate() {
        setCancelable(true);
        setCanceledOnTouchOutside(true);

        super.onCreate();
		L.v("OptionsDialog is created");
	}
	
	class AuthorItem {
		EditText editor;
		String value;
	}
	class AuthorList {
		String oldValue;
		ArrayList<AuthorItem> authorItems = new ArrayList<AuthorItem>();
		ViewGroup parent;
		void adjustEditors(AuthorItem item, boolean empty) {
			int index = authorItems.indexOf(item);
			if (empty) {
				// remove extra empty lines
				for (int i=authorItems.size() - 1; i >= 0; i--) {
					if (index == i)
						continue;
					AuthorItem v = authorItems.get(i); 
					if (v.value.length() == 0) {
						parent.removeView(v.editor);
						authorItems.remove(i);
					}
				}
			} else {
				// add one empty line, if none
				boolean found = false;
				for (int i=authorItems.size() - 1; i >= 0; i--) {
					if (index == i)
						continue;
					AuthorItem v = authorItems.get(i); 
					if (v.value.length() == 0) {
						found = true;
					}
				}
				if (!found) {
					add("");
				}
			}
		}
		void add(String value) {
			final AuthorItem item = new AuthorItem();
			item.editor = new EditText(getContext());
			item.value = value != null ? value : "";
			authorItems.add(item);
			parent.addView(item.editor);
			item.editor.addTextChangedListener(new TextWatcher() {
				@Override
				public void onTextChanged(CharSequence s, int start, int before, int count) {
					boolean oldValueEmpty = item.value == null || item.value.length() == 0;
					item.value = String.valueOf(s);
					boolean newValueEmpty = item.value == null || item.value.length() == 0;
					if (oldValueEmpty != newValueEmpty) {
						adjustEditors(item, newValueEmpty);
					}
				}
				
				@Override
				public void beforeTextChanged(CharSequence s, int start, int count,
						int after) {
				}
				
				@Override
				public void afterTextChanged(Editable s) {
				}
			});
		}
		public AuthorList(ViewGroup parent, String authors) {
			this.oldValue = authors;
			this.parent = parent;
	        parent.removeAllViews();
	        String[] list = authors.split("\\|");
	        for (String author : list)
	        	add(author);
			add("");
		}
	}

	
	AuthorList authors;
	@Override
	protected void onCreate(Bundle savedInstanceState) {
		super.onCreate(savedInstanceState);

        mInflater = LayoutInflater.from(getContext());
        FileInfo file = mBookInfo.getFileInfo();
        ViewGroup view = (ViewGroup)mInflater.inflate(R.layout.book_info_edit_dialog, null);
        EditText edTitle = (EditText)view.findViewById(R.id.book_title);
        //EditText edAuthor = (EditText)view.findViewById(R.id.book_author);
        EditText edSeriesName = (EditText)view.findViewById(R.id.book_series_name);
        EditText edSeriesNumber = (EditText)view.findViewById(R.id.book_series_number);
        RatingBar rbBookRating = (RatingBar)view.findViewById(R.id.book_rating);
        RadioGroup rgState = (RadioGroup)view.findViewById(R.id.book_state);
        int state = file.getReadingState();
        int[] stateButtons = new int[] {R.id.book_state_new, R.id.book_state_toread, R.id.book_state_reading, R.id.book_state_finished};
        rgState.check(state >= 0 && state < stateButtons.length ? stateButtons[state] : R.id.book_state_new);
        final ImageView image = (ImageView)view.findViewById(R.id.book_cover);
        int w = mWindowSize * 4 / 10;
        int h = w * 4 / 3;
        image.setMinimumHeight(h);
        image.setMaxHeight(h);
        image.setMinimumWidth(w);
        image.setMaxWidth(w);
        Bitmap bmp = Bitmap.createBitmap(w, h, Config.RGB_565);
        mActivity.getBrowser().getCoverpageManager().drawCoverpageFor(file, bmp, new CoverpageBitmapReadyListener() {
			@Override
			public void onCoverpageReady(FileInfo file, Bitmap bitmap) {
		        BitmapDrawable drawable = new BitmapDrawable(bitmap);
				image.setImageDrawable(drawable);
			}
		}); 
//        Spinner spStateSpinner = (Spinner)view.findViewById(R.id.book_state);
//        ArrayAdapter<String> adapter = new ArrayAdapter<String>(getContext(), R.layout.book_state_item, new String[] {
//        	"No mark",
//        	"To read",
//        	"Reading",
//        	"Finished",
//        });
//        spStateSpinner.setAdapter(adapter);
        LinearLayout llBookAuthorsList = (LinearLayout)view.findViewById(R.id.book_authors_list);
//        spStateSpinner.setAdapter(new SpinnerAdapter() {
//			@Override
//			public void unregisterDataSetObserver(DataSetObserver observer) {
//			}
//			
//			@Override
//			public void registerDataSetObserver(DataSetObserver observer) {
//			}
//			
//			@Override
//			public boolean isEmpty() {
//				return false;
//			}
//			
//			@Override
//			public boolean hasStableIds() {
//				return true;
//			}
//			
//			@Override
//			public int getViewTypeCount() {
//				return 1;
//			}
//			
//			@Override
//			public View getView(int position, View convertView, ViewGroup parent) {
//				// TODO Auto-generated method stub
//				return null;
//			}
//			
//			@Override
//			public int getItemViewType(int position) {
//				// TODO Auto-generated method stub
//				return 0;
//			}
//			
//			@Override
//			public long getItemId(int position) {
//				// TODO Auto-generated method stub
//				return 0;
//			}
//			
//			@Override
//			public Object getItem(int position) {
//				// TODO Auto-generated method stub
//				return null;
//			}
//			
//			@Override
//			public int getCount() {
//				// TODO Auto-generated method stub
//				return 0;
//			}
//			
//			@Override
//			public View getDropDownView(int position, View convertView, ViewGroup parent) {
//				// TODO Auto-generated method stub
//				return null;
//			}
//		});
        
        edTitle.setText(file.title);
        authors = new AuthorList(llBookAuthorsList, file.authors);
        //edAuthor.setText(file.authors);
        edSeriesName.setText(file.series);
        if (file.series != null && file.series.length() > 0 && file.seriesNumber > 0)
        	edSeriesNumber.setText(String.valueOf(file.seriesNumber));
        rbBookRating.setRating(file.getRate());
        setView(view);
	}
	
	
}
