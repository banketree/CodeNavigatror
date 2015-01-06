package com.fsti.ladder.common;

import java.io.File;
import java.lang.ref.SoftReference;
import java.util.ArrayList;
import java.util.HashMap;
import java.util.List;
import android.app.Activity;
import android.content.Context;
import android.graphics.Bitmap;
import android.view.View;
import android.view.ViewGroup.LayoutParams;

import com.fsti.ladder.R;
import com.fsti.ladder.adapter.ViewFlowAdapter;
import com.fsti.ladder.tools.BitmapUtils;
import com.fsti.ladder.tools.FileUtils;
import com.fsti.ladder.tools.LogUtils;
import com.fsti.ladder.widget.CircleFlowIndicator;
import com.fsti.ladder.widget.ViewFlow;

public class AdvertApi {
	private Context mContext;
	private ViewFlow mViewFlow;// 导航条
	private CircleFlowIndicator mIndicator;// 广告数目点
	private ViewFlowAdapter mViewFlowAdapter;
	HashMap<String, SoftReference<Bitmap>> cache;
	public AdvertApi(Activity activity){
		mContext= activity;
		mViewFlow = (ViewFlow) activity.findViewById(R.id.viewflow);
		mIndicator = (CircleFlowIndicator) activity.findViewById(R.id.viewflowindic);
		mViewFlow.setFlowIndicator(mIndicator);
		cache = new HashMap<String, SoftReference<Bitmap>>();
	}
	private void setDefaultAdvert(int type){
		ArrayList<Integer> lstAdvertImg = new ArrayList<Integer>();
		if(type == 0){
    		lstAdvertImg.add(R.drawable.default_advert_first);
    		lstAdvertImg.add(R.drawable.default_advert_second);
		}else{
		    lstAdvertImg.add(R.drawable.adv_call_first);
		}
		mViewFlowAdapter = new ViewFlowAdapter(mContext, lstAdvertImg);
		mViewFlow.setAdapter(mViewFlowAdapter);
		mViewFlow.setmSideBuffer(lstAdvertImg.size());
		mViewFlow.setTimeSpan(10000);
		mViewFlow.setSelection(2 * 100);
		mViewFlow.startAutoFlowTimer();
	}
	private void setAdvertList(List<Bitmap> list, int sideBuffer, int timeSpan, int selection,boolean isAutoFlow){
        mViewFlowAdapter = new ViewFlowAdapter(mContext, list);
        mViewFlow.setAdapter(mViewFlowAdapter);
	    mViewFlow.setmSideBuffer(sideBuffer);
	    LayoutParams params = mIndicator.getLayoutParams();
	    params.width =  mIndicator.getWidthResult(sideBuffer);
	    mIndicator.setLayoutParams(params);
	    mViewFlow.setTimeSpan(timeSpan);
	    mViewFlow.setSelection(selection);
	    if(isAutoFlow){
	        mViewFlow.startAutoFlowTimer();
	    }else{
	        mViewFlow.stopAutoFlowTimer();
	    }	    
	}
	
	public void stopAutoFlow()
	{
		if(mViewFlow != null)
		{
			mViewFlow.stopAutoFlowTimer();
		}
	}
	
	public void setAdvertViewFlow(String path,int type){
	    LogUtils.printLog("path:"+path);
	    String[] advertPaths = FileUtils.getFilePaths(path);	    
	    if(advertPaths != null && advertPaths.length >0){
	        List<Bitmap> lstImgAdvert = new ArrayList<Bitmap>();
            for (String imgPath : advertPaths) {
                LogUtils.printLog("advertPaths:" + imgPath);
                String picPath = path + File.separator + imgPath;
                Bitmap bitmap = getBitmapFromCache(picPath);
                if(bitmap != null){
                	lstImgAdvert.add(bitmap);
                }else{
                	bitmap =BitmapUtils.getBitmap(picPath);
                	lstImgAdvert.add(bitmap);
                	cache.put(picPath, new SoftReference<Bitmap>(bitmap));
                }          
            }
            setAdvertList(lstImgAdvert, lstImgAdvert.size(), 10000, 0, true);
	    }else{
	        setDefaultAdvert(type);
	    }        
	}
	
	public void recycleBitmap(){
	}
	
	public Bitmap getBitmapFromCache(String url) {
		Bitmap bitmap = null;
		if (cache.containsKey(url)) {
			bitmap = cache.get(url).get();
		}
		return bitmap;
	}
	
}

