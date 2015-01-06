package com.fsti.ladder.adapter;

import java.util.List;
import android.content.Context;
import android.graphics.Bitmap;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.widget.BaseAdapter;
import android.widget.ImageView;
import com.fsti.ladder.R;

public class ViewFlowAdapter extends BaseAdapter {
	private Context mContext;
	private LayoutInflater mInflater;
	public List<?> lstImage;

	public ViewFlowAdapter(Context context,List<?> list) {
		mContext = context;
		lstImage = list;
		mInflater = (LayoutInflater) context.getSystemService(Context.LAYOUT_INFLATER_SERVICE);
	}

	@Override
	public int getCount() {
		// TODO Auto-generated method stub
		return Integer.MAX_VALUE;
	}

	@Override
	public Object getItem(int position) {
		// TODO Auto-generated method stub
		return lstImage.get(position);
	}

	@Override
	public long getItemId(int position) {
		// TODO Auto-generated method stub
		return position;
	}

	@Override
	public View getView(int position, View convertView, ViewGroup parent) {
		// TODO Auto-generated method stub
	    ViewHolder holder = null;
		if (convertView == null) {
			convertView = mInflater.inflate(R.layout.advert_item, null);
			holder = new ViewHolder();
			holder.imgView = (ImageView) convertView.findViewById(R.id.imgView);
			convertView.setTag(holder);
		}else{
		    holder = (ViewHolder) convertView.getTag();
		}
		int lstSize = lstImage.size();
		Object obj = lstImage.get(position%lstSize);
		if(obj instanceof Integer){
		    holder.imgView.setImageResource((Integer) lstImage.get(position % lstSize));
		}else if(obj instanceof Bitmap){
		    holder.imgView.setImageBitmap((Bitmap) lstImage.get(position % lstSize));
		}
		return convertView;
	}
	static class ViewHolder{
	    ImageView imgView;
	}

}
