package com.fsti.ladder.adapter;

import java.util.ArrayList;
import android.content.Context;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.widget.BaseAdapter;
import android.widget.TextView;
import com.fsti.ladder.R;
import com.fsti.ladder.bean.RelativeInfo;
import com.fsti.ladder.tools.LogUtils;

public class RelativeInfoAdapter extends BaseAdapter {
    private ArrayList<RelativeInfo> lstDeviceInfo;
    private Context mContext;
    private LayoutInflater mInflater;
    public RelativeInfoAdapter(Context ctx, ArrayList<RelativeInfo> list){
        mContext = ctx;
        lstDeviceInfo = list;
        mInflater = LayoutInflater.from(ctx);
        
    }
    @Override
    public int getCount() {
        // TODO Auto-generated method stub
        return lstDeviceInfo.size();
    }

    @Override
    public Object getItem(int position) {
        // TODO Auto-generated method stub
        return lstDeviceInfo.get(position);
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
        if(convertView == null){
            convertView = mInflater.inflate(R.layout.layout_device_info_item, null);
            holder = new ViewHolder();
            holder.tv_device_name = (TextView) convertView.findViewById(R.id.tv_device_item_name);
            holder.tv_device_value = (TextView) convertView.findViewById(R.id.tv_device_item_value);
            convertView.setTag(holder);
        }else{
            holder = (ViewHolder) convertView.getTag();
        }
        RelativeInfo deviceInfo = lstDeviceInfo.get(position);
        holder.tv_device_name.setText(deviceInfo.itemName);
        LogUtils.printLog("itemValue:"+deviceInfo.itemValue);
        holder.tv_device_value.setText(deviceInfo.itemValue);
        return convertView;
    }
    
    private class ViewHolder{
        TextView tv_device_name;
        TextView tv_device_value;
    }
}
