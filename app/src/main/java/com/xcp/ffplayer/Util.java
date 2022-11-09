package com.xcp.ffplayer;

/**
 * Created by 许成谱 on 2022/11/9 17:09.
 * qq:1550540124
 * 热爱生活每一天！
 */
public class Util {

   public static String getMinutes(int duration) { // 给我一个duration，转换成xxx分钟
      int minutes = duration / 60;
      if (minutes <= 9) {
         return "0" + minutes;
      }
      return "" + minutes;
   }

   public static String getSeconds(int duration) { // 给我一个duration，转换成xxx秒
      int seconds = duration % 60;
      if (seconds <= 9) {
         return "0" + seconds;
      }
      return "" + seconds;
   }
}
