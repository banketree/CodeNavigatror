package com.fsti.ladder.adapter;

import android.content.Context;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.widget.BaseAdapter;
import android.widget.TextView;
import com.fsti.ladder.R;

/**
 * ��Ȩ���� (c)2012, �ʿ�
 * <p>
 * �ļ����� ��SettingsAdapter.java
 * <p>
 * ����ժҪ ��adapter for settings activity to choice settings item ���� �������� ����ʱ��
 * ��2012-12-31 ����11:33:32 ��ǰ�汾�ţ�v1.0 ��ʷ��¼ : ���� : 2012-12-31 ����11:33:32 �޸��ˣ� ���� :
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
