package com.fsti.ladder.adapter;

import android.content.Context;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.widget.BaseAdapter;
import android.widget.TextView;
import com.fsti.ladder.R;

/**
 * 版权所有 (c)2012, 邮科
 * <p>
 * 文件名称 ：SettingsAdapter.java
 * <p>
 * 内容摘要 ：adapter for settings activity to choice settings item 作者 ：林利澍 创建时间
 * ：2012-12-31 上午11:33:32 当前版本号：v1.0 历史记录 : 日期 : 2012-12-31 上午11:33:32 修改人： 描述 :
 */
public class OptionAdapter extends BaseAdapter {
    private LayoutInflater mInflater;
    private String[] strArray;
    private int[] background_imgs = {R.drawable.config_settings_item_bg1, R.drawable.config_settings_item_bg2, 
            R.drawable.config_settings_item_bg3, R.drawable.config_settings_item_bg4, R.drawable.config_settings_item_bg5, 
            R.drawable.config_settings_item_bg6,R.drawable.config_settings_item_bg7,R.drawable.config_settings_item_bg8,
            R.drawable.config_settings_item_bg9};

    public OptionAdapter(Context context, String[] str) {
        strArray = str;
        mInflater = LayoutInflater.from(context);
    }

    @Override
    public int getCount() {
        // TODO Auto-generated method stub
        return strArray.length;
    }

    @Override
    public Object getItem(int position) {
        // TODO Auto-generated method stub
        return strArray[position];
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
            convertView = mInflater.inflate(R.layout.activity_config_settings_item, null);
            holder = new ViewHolder();
            holder.tvSettingsOption = (TextView) convertView.findViewById(R.id.tvSettingsOption);
            convertView.setTag(holder);
        }else{
            holder = (ViewHolder) convertView.getTag();
        }        
        holder.tvSettingsOption.setText(strArray[position]);
        holder.tvSettingsOption.setBackgroundResource(background_imgs[position]);
        return convertView;
    }
    private class ViewHolder{
        TextView tvSettingsOption;
    }
}
