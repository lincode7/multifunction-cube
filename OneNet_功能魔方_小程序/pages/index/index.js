// index.js
var app = getApp();

Page({
  data: {
    device_id: app.globalData.device_id,
    link_staue: false,
    trans_protocol: null,
    datastreams: null,

    motto: 'OneNet',
    userInfo: {},
    hasUserInfo: false,
    canIUse: wx.canIUse('button.open-type.getUserInfo'),
    canIUseGetUserProfile: false,
    canIUseOpenData: wx.canIUse('open-data.type.userAvatarUrl') && wx.canIUse('open-data.type.userNickName') // 如需尝试获取用户信息可改为false
  },

  

  onLoad() {
    if (wx.getUserProfile) {
      this.setData({
        canIUseGetUserProfile: true
      })
    }
    this.getDeviceStaue();
  },

  onShow() {
    var that = this;
    this.data.realTime = setInterval(function () {
        console.log("刷新页面");
        that.getDeviceStaue();
    }, 600000) //间隔时间
},

  // 事件处理函数
  bindViewTap() {
    wx.navigateTo({
      url: '../logs/logs'
    })
  },
  getUserProfile(e) {
    // 推荐使用wx.getUserProfile获取用户信息，开发者每次通过该接口获取用户个人信息均需用户确认，开发者妥善保管用户快速填写的头像昵称，避免重复弹窗
    wx.getUserProfile({
      desc: '展示用户信息', // 声明获取用户个人信息后的用途，后续会展示在弹窗中，请谨慎填写
      success: (res) => {
        console.log(res)
        this.setData({
          userInfo: res.userInfo,
          hasUserInfo: true
        })
      }
    })
  },
  getUserInfo(e) {
    // 不推荐使用getUserInfo获取用户信息，预计自2021年4月13日起，getUserInfo将不再弹出弹窗，并直接返回匿名的用户个人信息
    console.log(e)
    this.setData({
      userInfo: e.detail.userInfo,
      hasUserInfo: true
    })
  },
  // 查询设备信息
  getDeviceStaue: function () {
      wx.request({
        url: 'http://api.heclouds.com/devices/' + app.globalData.device_id,
        method: 'GET',
        header:{
          'content-type': 'application/x-www-form-urlencoded',
          "api-key": app.globalData.api_key
        },
        success: res => {
          this.setData({
            link_staue: res.data.data.online,
            trans_protocol: res.data.data.protocol,
            datastreams: res.data.data.datastreams
          })
          app.globalData.datastreams = res.data.data.datastreams;
        }
      })
  },

})