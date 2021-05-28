// app.js

App({
  onLaunch() {
    // 展示本地存储能力
    const logs = wx.getStorageSync('logs') || []
    logs.unshift(Date.now())
    wx.setStorageSync('logs', logs)

    // 登录
    wx.login({
      success: res => {
        // 发送 res.code 到后台换取 openId, sessionKey, unionId
      }
    })
  },
  globalData: {
    // 全局变量
    userInfo: null,

    device_id: '709577224',
    api_key: 'WiDIqLodYI4jGEYP5TVH1aZJeKk=',
    datastreams: null,
  },
})


//封装 http
module.exports= (url, path,method, params)=>{
  return new Promise((resolve, reject) => {
    wx.request({
      url: `${url}/${path}`,
      method:method,
      data: Object.assign({}, params),
      header: { 'Content-Type': 'application/text' },
      success: resolve,
      fail: reject
    })
  })
}