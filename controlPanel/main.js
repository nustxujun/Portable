// Modules to control application life and create native browser window
const { app, BrowserWindow,ipcMain } = require('electron')
const url = require('url')
const path = require('path')
var ref = require("ref");

// Call *.dll with ffi
let ffi = require('ffi');
let dll = ffi.Library('Portable.dll', {
	'init': ['void', ['int','int','int','int','uint32']],
	'paint': ['void', ['string', 'uint64']],
	'setCallback':['void',['pointer']],
	'handleEvent':['void',['string', 'uint64']],
	'close':['void',[]]
})


// Keep a global reference of the window object, if you don't, the window will
// be closed automatically when the JavaScript object is garbage collected.
let mainWindow;
let callback

var height = 800;

app.disableHardwareAcceleration();

function createWindow() {
	// Create the browser window.
	mainWindow = new BrowserWindow({
		// show: false,
		// transparent: true,
		width: 400, 
		height: height,
		webPreferences: {
			nodeIntegration: true,
			offscreen: true,
		}
	})

	// and load the index.html of the app.
	mainWindow.loadURL(url.format({
		pathname: path.join(__dirname, 'index.html'),
		protocol: 'file:',
		slashes: true
	}))
	// Open the DevTools.
	// mainWindow.webContents.openDevTools()


	
	callback = ffi.Callback('void', ['string', 'uint64'],
	function(msg, size) {
		var data = JSON.parse(msg)

		if (data.mouse)
		{
			mainWindow.webContents.sendInputEvent(data.mouse);
		}
		else
		{			
			mainWindow.webContents.send('renderer', data);
		}
	});
	dll.setCallback(callback)




	// Emitted when the window is closed.
	mainWindow.on('closed', function () {
		dll.setCallback(null);
		dll.close();
		// Dereference the window object, usually you would store windows
		// in an array if your app supports multi windows, this is the time
		// when you should delete the corresponding element.
		mainWindow = null
	})

	mainWindow.webContents.on('paint', (event, dirty, image) => {
		var buffer = image.getBitmap();
		dll.paint(buffer,buffer.length);
	})

	
	mainWindow.webContents.setFrameRate(60);

	ipcMain.on('main',function(event, msg)
	{
		var data = JSON.stringify(msg);
		dll.handleEvent(data, data.length);
	});

	ipcMain.once('renderer-loaded', () => {
		var handle = mainWindow.getNativeWindowHandle();
		dll.init(1600, height,400, height,handle.readUInt32LE());
	})


}

// This method will be called when Electron has finished
// initialization and is ready to create browser windows.
// Some APIs can only be used after this event occurs.
app.on('ready', createWindow)

// Quit when all windows are closed.
app.on('window-all-closed', function () {
	// On macOS it is common for applications and their menu bar
	// to stay active until the user quits explicitly with Cmd + Q
	if (process.platform !== 'darwin') app.quit()
})

app.on('activate', function () {
	// On macOS it's common to re-create a window in the app when the
	// dock icon is clicked and there are no other windows open.
	if (mainWindow === null) createWindow()
})

// In this file you can include the rest of your app's specific main process
// code. You can also put them in separate files and require them here.

// var network = require("./Network.js");
// network.init(8888);
// network.receive(function (data) {

// })