// pages/LED/LED.js
const app = getApp()

Page({

    data: {
        streamdata: null,
        // led状态
        redledstatus: false,
        yellowledstatus: false,
        greenledstatus: false,
        blueledstatus: false,
        all_staue: false,

        equiplist: ['{红色LED}:', '{黄色LED}:', '{蓝色LED}:', '{绿色LED}:']
    },

    onLoad: function (options) {
        this.getLEDStatue();
    },

    onShow: function () {
        var that = this;
        this.data.realTime = setInterval(function () {
            console.log("刷新页面");
            that.getLEDStatue();
        }, 5000) //间隔时间
    },

    allcheck: function () {
        if (this.redledstatus && this.yellowledstatus && this.blueledstatus && this.greenledstatus && this.beepstatus) {
            this.setData({
                allstatus: true
            })
        }
    },
    allchange: function (event) {
        var status = event.detail.value
        this.setData({
            redledstatus: status,
            yellowledstatus: status,
            greenledstatus: status,
            blueledstatus: status,
            beepstatus: status,
            allstatus: status
        })
        if (status) {
            this.operateAll('1')
        } else {
            this.operateAll('0')
        }
    },
    redledchange: function (event) {
        var status = event.detail.value
        this.setData({
            redledstatus: status
        })

        if (this.data.redledstatus) {
            var command = '{红色LED}:1'
            this.operate(command)
        } else {
            var command = '{红色LED}:0'
            this.operate(command)
        }
        this.allcheck()
    },
    yellowledchange: function (event) {
        var status = event.detail.value
        this.setData({
            yellowledstatus: status
        })

        if (this.data.yellowledstatus) {
            var command = '{黄色LED}:1'
            this.operate(command)
        } else {
            var command = '{黄色LED}:0'
            this.operate(command)
        }
        this.allcheck()
    },
    blueledchange: function (event) {
        var status = event.detail.value
        this.setData({
            blueledstatus: status
        })

        if (this.data.blueledstatus) {
            var command = '{蓝色LED}:1'
            this.operate(command)
        } else {
            var command = '{蓝色LED}:0'
            this.operate(command)
        }
        this.allcheck()
    },
    greenledchange: function (event) {
        var status = event.detail.value
        this.setData({
            greenledstatus: status
        })

        if (this.data.greenledstatus) {
            var command = '{绿色LED}:1'
            this.operate(command)
        } else {
            var command = '{绿色LED}:0'
            this.operate(command)
        }
        this.allcheck()
    },
    operateAll:function(id){
        var i = 0
        for(i=0;i<this.data.equiplist.length;i++){
          var command = this.data.equiplist[i]+id
          this.operate(command)
        }
      },
    operate: function (comm) {
        wx.request({
            url: 'http://api.heclouds.com/cmds?device_id=' + app.globalData.device_id,
            method: 'POST',
            header: {
                'content-type': 'application/x-www-form-urlencoded',
                "api-key": app.globalData.apikey
            },
            data: comm,
            success(res) {
                // console.log(res)
            }
        });
        setTimeout(function () {
            //要延时执行的代码
           }, 5000) //延迟时间 这里是5秒
    },
    getLEDStatue: function () {
        wx.request({
            url: 'http://api.heclouds.com/devices/' + app.globalData.device_id + '/datastreams?datastream_ids=红色LED,绿色LED,黄色LED,蓝色LED',
            header: {
                'content-type': 'application/x-www-form-urlencoded',
                "api-key": app.globalData.api_key
            },
            success: res => {
                this.setData({
                    streamdata: res.data.data,
                })
                for (var i = 0; i < this.data.streamdata.length; i++) {
                    if (this.data.streamdata[i].id.localeCompare("红色LED") == 0) {
                        this.setData({
                            redledstatus: this.data.streamdata[i].current_value,
                        })
                    } else if (this.data.streamdata[i].id.localeCompare("绿色LED") == 0) {
                        this.setData({
                            greenledstatus: this.data.streamdata[i].current_value,
                        })
                    } else if (this.data.streamdata[i].id.localeCompare("黄色LED") == 0) {
                        this.setData({
                            yellowledstatus: this.data.streamdata[i].current_value,
                        })
                    } else if (this.data.streamdata[i].id.localeCompare("蓝色LED") == 0) {
                        this.setData({
                            blueledstatus: this.data.streamdata[i].current_value,
                        })
                    }
                }
                this.allcheck();
            }
        })
    },
})