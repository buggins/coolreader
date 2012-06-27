package org.coolreader.crengine;

import java.util.ArrayList;

import org.coolreader.R;

import android.content.ComponentName;
import android.content.Context;
import android.content.Intent;
import android.graphics.Rect;
import android.graphics.drawable.Drawable;
import android.view.ContextMenu.ContextMenuInfo;
import android.view.Gravity;
import android.view.KeyEvent;
import android.view.Menu;
import android.view.MenuItem;
import android.view.MotionEvent;
import android.view.SubMenu;
import android.view.View;
import android.view.ViewGroup;
import android.view.WindowManager;
import android.widget.ImageButton;
import android.widget.PopupWindow;

public class CRToolBar extends ViewGroup {

	public CRToolBar(Context context) {
		super(context);
	}

	@Override
	protected void onLayout(boolean changed, int l, int t, int r, int b) {
		
	}
	
//	private final Menu menu;
//	private boolean showLabels;
//	private int contentHeight;
//	private int buttonHeight;
//	private int buttonWidth;
//	private int visibleButtonCount;
//	private int visibleNonButtonCount;
//	public CRToolBar(Context context, Menu menu) {
//		super(context);
//		this.menu = menu;
//		this.showLabels = true;
//		for (int i=0; i<menu.size(); i++) {
//			MenuItem item = menu.getItem(i);
//			if (!item.isVisible())
//				continue;
//			Drawable d = item.getIcon();
//			if (d == null) {
//				visibleNonButtonCount++;
//				continue;
//			}
//			visibleButtonCount++;
//			int w = d.getIntrinsicWidth() + 4;
//			int h = d.getIntrinsicHeight();
//			if (buttonWidth < w || buttonHeight < h) {
//				buttonWidth = w;
//				buttonHeight = h;
//				contentHeight = buttonHeight + getPaddingTop() + getPaddingBottom();
//			}
//			
//		}
//	}
//
//	private OnItemSelectedHandler onItemSelectedHandler;
//	
//	public void setOnItemSelectedHandler(OnItemSelectedHandler handler) {
//		this.onItemSelectedHandler = handler;
//	}
//	
//	public interface OnItemSelectedHandler {
//		boolean onOptionsItemSelected(MenuItem item);
//	}
//	
//	private void onMoreButtonClick() {
//		// TODO: show additional items
//	}
//	
//	private void onButtonClick(MenuItem item) {
//		if (onItemSelectedHandler != null)
//			onItemSelectedHandler.onOptionsItemSelected(item);
//	}
//	
//	private ImageButton addButton(Rect rect, final MenuItem item, boolean left) {
//		Rect rc = new Rect(rect);
//		if (left) {
//			rc.right = rc.left + buttonWidth;
//			rect.left += buttonWidth;
//		} else {
//			rc.left = rc.right - buttonWidth;
//			rect.right -= buttonWidth;
//		}
//		if (rc.isEmpty())
//			return null;
//		ImageButton ib = new ImageButton(getContext());
//		if (item != null) {
//			ib.setImageDrawable(item.getIcon());
//			ib.setTag(item);
//		} else {
//			ib.setImageDrawable(getResources().getDrawable(R.drawable.ic_menu_moreoverflow));
//		}
//		ib.setBackgroundDrawable(null);
//		ib.layout(rc.left, rc.top, rc.right, rc.bottom);
//		ib.setOnClickListener(new OnClickListener() {
//			@Override
//			public void onClick(View v) {
//				if (item != null)
//					onButtonClick(item);
//				else
//					onMoreButtonClick();
//			}
//		});
//		addView(ib);
//		return ib;
//	}
//	
//	@Override
//	protected void onLayout(boolean changed, int left, int top, int right,
//			int bottom) {
//		Rect rect = new Rect(left + getPaddingLeft(), top + getPaddingTop(), right +  + getPaddingRight(), bottom + getPaddingBottom());
//		if (rect.isEmpty())
//			return;
//		removeAllViews();
//		ArrayList<MenuItem> itemsToShow = new ArrayList<MenuItem>();
//		int maxWidth = right - left - getPaddingLeft() - getPaddingRight();
//		int maxButtonCount = maxWidth / buttonWidth;
//		int count = 0;
//		boolean addEllipsis = visibleButtonCount > maxButtonCount || visibleNonButtonCount > 0;
//		if (addEllipsis) {
//			addButton(rect, null, false);
//		}
//		for (int i = 0; i < menu.size(); i++) {
//			if (count >= maxButtonCount - 1)
//				break;
//			MenuItem item = menu.getItem(i);
//			if (!item.isVisible())
//				continue;
//			itemsToShow.add(item);
//			count++;
//			addButton(rect, item, true);
//		}
//	}
//
//	@Override
//	protected void onMeasure(int widthMeasureSpec, int heightMeasureSpec) {
//        int widthMode = MeasureSpec.getMode(widthMeasureSpec);
////        if (widthMode != MeasureSpec.EXACTLY) {
////            throw new IllegalStateException(getClass().getSimpleName() + " can only be used " +
////                    "with android:layout_width=\"match_parent\" (or fill_parent)");
////        }
//        
//        int heightMode = MeasureSpec.getMode(heightMeasureSpec);
////        if (heightMode != MeasureSpec.AT_MOST) {
////            throw new IllegalStateException(getClass().getSimpleName() + " can only be used " +
////                    "with android:layout_height=\"wrap_content\"");
////        }
//
//        int contentWidth = MeasureSpec.getSize(widthMeasureSpec);
//
//        int maxHeight = contentHeight > 0 ?
//                contentHeight : MeasureSpec.getSize(heightMeasureSpec);
//        setMeasuredDimension(contentWidth, maxHeight);
//	}
//
//	@Override
//	protected void onSizeChanged(int w, int h, int oldw, int oldh) {
//		super.onSizeChanged(w, h, oldw, oldh);
//	}
//
//	public static class MyMenuItem implements MenuItem {
//
//		char alphabeticShortcut;
//		char numericShortcut;
//		int groupId;
//		int itemId;
//		int order;
//		CharSequence title;
//		CharSequence titleCondensed;
//		Drawable icon;
//		SubMenu subMenu;
//		boolean checkable;
//		boolean checked;
//		boolean enabled = true;
//		boolean visible = true;
//		OnMenuItemClickListener listener;
//		Context context;
//		
//		public MyMenuItem(Context context) {
//			this.context = context;
//		}
//		
//		@Override
//		public char getAlphabeticShortcut() {
//			return alphabeticShortcut;
//		}
//
//		@Override
//		public int getGroupId() {
//			return groupId;
//		}
//
//		@Override
//		public Drawable getIcon() {
//			return icon;
//		}
//
//		@Override
//		public Intent getIntent() {
//			return null;
//		}
//
//		@Override
//		public int getItemId() {
//			return itemId;
//		}
//
//		@Override
//		public ContextMenuInfo getMenuInfo() {
//			return null;
//		}
//
//		@Override
//		public char getNumericShortcut() {
//			return numericShortcut;
//		}
//
//		@Override
//		public int getOrder() {
//			return order;
//		}
//
//		@Override
//		public SubMenu getSubMenu() {
//			return subMenu;
//		}
//
//		@Override
//		public CharSequence getTitle() {
//			return title;
//		}
//
//		@Override
//		public CharSequence getTitleCondensed() {
//			return titleCondensed;
//		}
//
//		@Override
//		public boolean hasSubMenu() {
//			return subMenu != null;
//		}
//
//		@Override
//		public boolean isCheckable() {
//			return checkable;
//		}
//
//		@Override
//		public boolean isChecked() {
//			return checked;
//		}
//
//		@Override
//		public boolean isEnabled() {
//			return enabled;
//		}
//
//		@Override
//		public boolean isVisible() {
//			return visible;
//		}
//
//		@Override
//		public MenuItem setAlphabeticShortcut(char alphaChar) {
//			alphabeticShortcut = alphaChar;
//			return this;
//		}
//
//		@Override
//		public MenuItem setCheckable(boolean checkable) {
//			this.checkable = checkable;
//			return this;
//		}
//
//		@Override
//		public MenuItem setChecked(boolean checked) {
//			this.checked = true;
//			return this;
//		}
//
//		@Override
//		public MenuItem setEnabled(boolean enabled) {
//			this.enabled = enabled;
//			return this;
//		}
//
//		@Override
//		public MenuItem setIcon(Drawable icon) {
//			this.icon = icon;
//			return this;
//		}
//
//		@Override
//		public MenuItem setIcon(int iconRes) {
//			if (iconRes != 0)
//				this.icon =  context.getResources().getDrawable(iconRes);
//			return this;
//		}
//
//		@Override
//		public MenuItem setIntent(Intent intent) {
//			// TODO
//			return this;
//		}
//
//		@Override
//		public MenuItem setNumericShortcut(char numericChar) {
//			numericShortcut = numericChar;
//			return this;
//		}
//
//		
//		@Override
//		public MenuItem setOnMenuItemClickListener(
//				OnMenuItemClickListener menuItemClickListener) {
//			this.listener = menuItemClickListener;
//			return this;
//		}
//
//		public MyMenuItem setOrder(int order) {
//			this.order = order;
//			return this;
//		}
//		
//		public MyMenuItem setItemId(int itemId) {
//			this.itemId = itemId;
//			return this;
//		}
//		
//		public MenuItem setGroupId(int groupId) {
//			this.groupId = groupId;
//			return this;
//		}
//		
//		public MenuItem setSubMenu(SubMenu subMenu) {
//			this.subMenu = subMenu;
//			return this;
//		}
//		
//		@Override
//		public MenuItem setShortcut(char numericChar, char alphaChar) {
//			this.numericShortcut = numericChar;
//			this.alphabeticShortcut = alphaChar;
//			return this;
//		}
//
//		@Override
//		public MenuItem setTitle(CharSequence title) {
//			this.title = title;
//			return this;
//		}
//
//		@Override
//		public MenuItem setTitle(int title) {
//			this.title = context.getResources().getString(title);
//			return this;
//		}
//
//		@Override
//		public MenuItem setTitleCondensed(CharSequence title) {
//			this.titleCondensed = title;
//			return this;
//		}
//
//		@Override
//		public MenuItem setVisible(boolean visible) {
//			this.visible = visible;
//			return this;
//		}
//		
//	}
//
//	public static class MyMenuBuilder implements SubMenu {
//
//		ArrayList<MenuItem> items = new ArrayList<MenuItem>();
//		Context context;
//		MenuItem parentItem;
//
//		private MenuItem add(MenuItem item) {
//			items.add(item);
//			return item;
//		}
//		
//		public MyMenuBuilder(Context context) {
//			this.context = context;
//		}
//		
//		@Override
//		public MenuItem add(CharSequence title) {
//			return add(new MyMenuItem(context).setTitle(title));
//		}
//
//		@Override
//		public MenuItem add(int titleRes) {
//			return add(new MyMenuItem(context).setTitle(titleRes));
//		}
//
//		@Override
//		public MenuItem add(int groupId, int itemId, int order,
//				CharSequence title) {
//			MyMenuItem item = new MyMenuItem(context); 
//			return add(item.setOrder(order).setItemId(itemId).setGroupId(groupId).setTitle(title));
//		}
//
//		@Override
//		public MenuItem add(int groupId, int itemId, int order, int titleRes) {
//			MyMenuItem item = new MyMenuItem(context); 
//			return add(item.setOrder(order).setItemId(itemId).setGroupId(groupId).setTitle(titleRes));
//		}
//
//		@Override
//		public int addIntentOptions(int groupId, int itemId, int order,
//				ComponentName caller, Intent[] specifics, Intent intent,
//				int flags, MenuItem[] outSpecificItems) {
//			return 0;
//		}
//
//		@Override
//		public SubMenu addSubMenu(CharSequence title) {
//			MyMenuItem item = new MyMenuItem(context);
//			item.setTitle(title);
//			MyMenuBuilder submenu = new MyMenuBuilder(context);
//			item.setSubMenu(submenu);
//			add(item);
//			submenu.parentItem = item;
//			return submenu;
//		}
//
//		@Override
//		public SubMenu addSubMenu(int titleRes) {
//			MyMenuItem item = new MyMenuItem(context);
//			item.setTitle(titleRes);
//			MyMenuBuilder submenu = new MyMenuBuilder(context);
//			item.setSubMenu(submenu);
//			add(item);
//			submenu.parentItem = item;
//			return submenu;
//		}
//
//		@Override
//		public SubMenu addSubMenu(int groupId, int itemId, int order,
//				CharSequence title) {
//			MyMenuItem item = new MyMenuItem(context);
//			item.setItemId(itemId);
//			item.setOrder(order);
//			item.setGroupId(groupId);
//			item.setTitle(title);
//			MyMenuBuilder submenu = new MyMenuBuilder(context);
//			item.setSubMenu(submenu);
//			add(item);
//			submenu.parentItem = item;
//			return submenu;
//		}
//
//		@Override
//		public SubMenu addSubMenu(int groupId, int itemId, int order,
//				int titleRes) {
//			MyMenuItem item = new MyMenuItem(context);
//			item.setItemId(itemId);
//			item.setOrder(order);
//			item.setGroupId(groupId);
//			item.setTitle(titleRes);
//			add(item);
//			MyMenuBuilder submenu = new MyMenuBuilder(context);
//			item.setSubMenu(submenu);
//			submenu.parentItem = item;
//			return submenu;
//		}
//
//		@Override
//		public void clear() {
//			items.clear();
//		}
//
//		@Override
//		public void close() {
//			// TODO
//		}
//
//		@Override
//		public MenuItem findItem(int id) {
//			for (MenuItem item : items)
//				if (item.getItemId() == id)
//					return item;
//			return null;
//		}
//
//		@Override
//		public MenuItem getItem(int index) {
//			return items.get(index);
//		}
//
//		@Override
//		public boolean hasVisibleItems() {
//			for (MenuItem item : items)
//				if (item.isVisible())
//					return true;
//			return false;
//		}
//
//		@Override
//		public boolean isShortcutKey(int keyCode, KeyEvent event) {
//			for (MenuItem item : items)
//				if (item.getAlphabeticShortcut() == event.getUnicodeChar() || item.getNumericShortcut() == event.getNumber())
//					return true;
//			return false;
//		}
//
//		@Override
//		public boolean performIdentifierAction(int id, int flags) {
//			return false;
//		}
//
//		@Override
//		public boolean performShortcut(int keyCode, KeyEvent event, int flags) {
//			return false;
//		}
//
//		@Override
//		public void removeGroup(int groupId) {
//		}
//
//		@Override
//		public void removeItem(int id) {
//			for (int i=0; i<items.size(); i++) {
//				if (items.get(i).getItemId() == id) {
//					items.remove(i);
//					return;
//				}
//			}
//		}
//
//		@Override
//		public void setGroupCheckable(int group, boolean checkable,
//				boolean exclusive) {
//		}
//
//		@Override
//		public void setGroupEnabled(int group, boolean enabled) {
//		}
//
//		@Override
//		public void setGroupVisible(int group, boolean visible) {
//		}
//
//		@Override
//		public void setQwertyMode(boolean isQwerty) {
//		}
//
//		@Override
//		public int size() {
//			return items.size();
//		}
//
//		@Override
//		public void clearHeader() {
//		}
//
//		@Override
//		public MenuItem getItem() {
//			return parentItem;
//		}
//
//		@Override
//		public SubMenu setHeaderIcon(int iconRes) {
//			return this;
//		}
//
//		@Override
//		public SubMenu setHeaderIcon(Drawable icon) {
//			return this;
//		}
//
//		@Override
//		public SubMenu setHeaderTitle(int titleRes) {
//			return this;
//		}
//
//		@Override
//		public SubMenu setHeaderTitle(CharSequence title) {
//			return this;
//		}
//
//		@Override
//		public SubMenu setHeaderView(View view) {
//			return this;
//		}
//
//		@Override
//		public SubMenu setIcon(int iconRes) {
//			return this;
//		}
//
//		@Override
//		public SubMenu setIcon(Drawable icon) {
//			return this;
//		}
//		
//	}
//	
//	public static PopupWindow showPopup(Context context, View anchor, Menu menu) {
//		final CRToolBar tb = new CRToolBar(context, menu);
//		tb.measure(anchor.getWidth(), ViewGroup.LayoutParams.WRAP_CONTENT);
//		int w = tb.getMeasuredWidth();
//		int р = tb.getMeasuredHeight();
//		final PopupWindow popup = new PopupWindow(context);
//		popup.setTouchInterceptor(new OnTouchListener() {
//			
//			@Override
//			public boolean onTouch(View v, MotionEvent event) {
//				if ( event.getAction()==MotionEvent.ACTION_OUTSIDE ) {
//					popup.dismiss();
//					return true;
//				}
//				return false;
//			}
//		});
//		//popup.setBackgroundDrawable(new BitmapDrawable());
//		popup.setWidth(WindowManager.LayoutParams.WRAP_CONTENT);
//		popup.setHeight(WindowManager.LayoutParams.WRAP_CONTENT);
//		popup.setFocusable(true);
//		popup.setTouchable(true);
//		popup.setOutsideTouchable(true);
//		popup.setContentView(tb);
//		int [] location = new int[2];
//		anchor.getLocationOnScreen(location);
//		int popupY = location[1];
//		popup.showAtLocation(anchor, Gravity.TOP | Gravity.LEFT, location[0], popupY);
//		return popup;
//	}
}
