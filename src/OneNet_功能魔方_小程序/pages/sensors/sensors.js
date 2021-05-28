// pages/sensors/sensors.js
var app = getApp();

Page({

    /**
     * 页面的初始数据
     */
    data: {
        streamdata: null,
        curtop: -1,
        showdata: null,
        img: null,
    },

    onLoad: function (options) {
        this.GetDataStream();
    },

    onShow: function () {
        var that = this;
        this.data.realTime = setInterval(function () {
            console.log("刷新页面");
            that.GetDataStream();
        }, 5000) //间隔时间
    },

    // 查询设备流数据
    GetDataStream: function () {
        wx.request({
            url: 'http://api.heclouds.com/devices/' + app.globalData.device_id + '/datastreams',
            header: {
                'content-type': 'application/x-www-form-urlencoded',
                "api-key": app.globalData.api_key
            },
            success: res => {
                this.setData({
                    streamdata: res.data.data,
                });
                console.log(res.data.data.online);
                for (var i = 0; i < this.data.streamdata.length; i++) {
                    if (this.data.streamdata[i].id.localeCompare("当前顶面") == 0) {
                        this.setData({
                            curtop: this.data.streamdata[i].current_value,
                        });
                        // console.log(this.data.curtop)
                        this.SelectData();
                        break;
                    }
                }
            }
        })
    },

    // 根据 当前顶面 处理数据
    SelectData: function () {
        switch (this.data.curtop) {
            case 0:
                this.setData({
                    img: "./img/top.png"
                })
                this.data.showdata = [];
                for (var i = 0; i < this.data.streamdata.length; i++) {
                    if (this.data.streamdata[i].id.localeCompare("温度") == 0 || this.data.streamdata[i].id.localeCompare("湿度") == 0) {
                        this.setData({
                            showdata: this.data.showdata.concat(this.data.streamdata[i])
                        })
                    }
                }
                break;
            case 1:
                this.setData({
                    img: "./img/bottom.png"
                })
                this.data.showdata = [];
                for (var i = 0; i < this.data.streamdata.length; i++) {
                    if (this.data.streamdata[i].id.localeCompare("计时器") == 0) {
                        this.setData({
                            showdata: this.data.showdata.concat(this.data.streamdata[i])
                        })
                    }
                }
                break;
            case 2:
                this.setData({
                    img: "./img/left.png"
                })
                this.data.showdata = [];
                for (var i = 0; i < this.data.streamdata.length; i++) {
                    if (this.data.streamdata[i].id.localeCompare("计时器") == 0) {
                        this.setData({
                            showdata: this.data.showdata.concat(this.data.streamdata[i])
                        })
                    }
                }
                break;
            case 3:
                this.setData({
                    img: "./img/right.png"
                })
                this.data.showdata = [];
                for (var i = 0; i < this.data.streamdata.length; i++) {
                    if (this.data.streamdata[i].id.localeCompare("计时器") == 0) {
                        this.setData({
                            showdata: this.data.showdata.concat(this.data.streamdata[i])
                        })
                    }
                }
                break;
            case 4:
                this.setData({
                    img: "./img/font.png"
                })
                this.data.showdata = [];
                for (var i = 0; i < this.data.streamdata.length; i++) {
                    if (this.data.streamdata[i].id.localeCompare("光强") == 0) {
                        this.setData({
                            showdata: this.data.showdata.concat(this.data.streamdata[i])
                        })
                    }
                }
                break;
            case 5:
                this.setData({
                    img: "./img/back.png"
                })
                this.data.showdata = [];
                for (var i = 0; i < this.data.streamdata.length; i++) {
                    if (this.data.streamdata[i].id.localeCompare("温度") == 0 || this.data.streamdata[i].id.localeCompare("湿度") == 0 || this.data.streamdata[i].id.localeCompare("温度报警") == 0) {
                        this.setData({
                            showdata: this.data.showdata.concat(this.data.streamdata[i])
                        })
                    }
                }
                break;
            default:
                this.setData({
                    img: "./img/unkonw.png"
                })
                break;
        }
    },
})