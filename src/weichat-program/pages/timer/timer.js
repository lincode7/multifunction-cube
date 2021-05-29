// pages/timer/timer.js
const app = getApp()

Page({

    /**
     * 页面的初始数据
     */
    data: {
        timer_s: null,
        timer_count: null,
        h: 0,
        m: 0,
        s: 0,
    },

    input_h: function (e) {
        this.setData({
            h: e.detail.value
        })
    },
    input_m: function (e) {
        this.setData({
            m: e.detail.value
        })
    },
    input_s: function (e) {
        this.setData({
            s: e.detail.value
        })
    },

    onLoad: function (options) {
        this.getTimer();
    },

    onShow: function () {
        var that = this;
        this.data.realTime = setInterval(function () {
            console.log("刷新页面");
            that.getTimer();
        }, 1000) //间隔时间
    },

    getTimer: function () {
        wx.request({
            url: 'http://api.heclouds.com/devices/' + app.globalData.device_id + '/datastreams/计时器',
            header: {
                'content-type': 'application/x-www-form-urlencoded',
                "api-key": app.globalData.api_key
            },
            success: res => {
                this.setData({
                    timer_count: res.data.data.current_value,
                });
                // var t = this.data.timer_count.split(":");
                // this.setData({
                //     h: t[0],
                //     m: t[1],
                //     s: t[2],
                //     timer_s: parseInt(t[0]) * 3600 + parseInt(t[1]) * 60 + parseInt(t[2])
                // });
                console.log(this.data.timer_count);
            }
        });
        setTimeout(function () {
            //要延时执行的代码
           }, 5000) //延迟时间 这里是5秒
    }
})