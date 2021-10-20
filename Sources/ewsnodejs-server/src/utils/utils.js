var fs = require('fs');
var path = require('path');
var multer = require('multer');
const { resolve } = require('path');
var storage = multer.diskStorage({
    destination: function (req, file, cb) {
    cb(null, mediaPath)
  },
  filename: function (req, file, cb) {
    cb(null, file.originalname )
  }
})

var upload = multer({ storage: storage }).array('file')

const videoExtension = '.mp4';
const audioExtension = '.wav';
const saltRounds = 10;

var mediaPath = null; // the main media folder
var videoFilePath = null; // store video contents
var audioFilePath = null; // store audio contents 
var mrtFilePath = null; // store MRT contents

/* set up the media folder file path */
const setPath = () => {
    if (process.env.OSP_MEDIA==undefined){
        // if OSP_MEDIA is not set, query from get_media
        //  this is not working yet
        mediaPath = get_media;
    }
    mediaPath = process.env.OSP_MEDIA + '/' ; 
    videoFilePath = process.env.OSP_MEDIA + '/video/';
    audioFilePath = process.env.OSP_MEDIA + '/audio/';
    mrtFilePath = process.env.OSP_MEDIA + "/MultiAFC/"; 
} 

console.log(" set up the media folder file path ... ");
setPath();

// TODO: we may want to add parameters here. 
const getFileNames = () => new Promise((resolve, reject) => {

    if (mediaPath==undefined) {
        throw "OSP_MEDIA is not set";
    }
    // currently, the path is hard-coded as mediaPath+"Freping"
    // this is because only Freping & CoarseFit use this method. 
    // Freping & CoarseFit apps display the same media contents in drop-down
    fs.readdir(mediaPath+"Freping", (err, filenames) => {
        if (err) {
            throw err;
        }
        resolve(filenames);
    });
});

/* retrieve MRT JSON filenames */
const getMRTJSON = () => new Promise((resolve, reject) => {
    if (mrtFilePath==undefined){
        throw "OSP_MEDIA is not set";
    }
    fs.readdir(mrtFilePath+"MRT", (err, filenames) => {
        if (err) {
            throw err;
        }
        resolve(filenames);
    });
});

const readJSONFile = (path) => new Promise((resolve, reject) => {
    fs.readFile(path, (err, data) => {
        if (err) {
            throw "Cannot read path";
        }
        let jsonData = JSON.parse(data);
        console.log(jsonData);
        resolve(jsonData);
    });
});

const uploadFiles = (req, res) => {
    upload(req, res, function (err) {
        if (err instanceof multer.MulterError) {
            res.status(500).json(err);
        } else if (err) {
            res.status(500).json(err);
        }
        res.status(200).send(req.file);
    });
}

const deleteFile = (filename) => {
    try {
        fs.unlinkSync(mediaPath + filename);
    } catch(err) {
        console.error(err)
        throw err;
    }
}

const getUserAssessmentConfig = () => {
    try {
        var data=fs.readFileSync(mediaPath + 'UserAssessmentConfig.json', 'utf8');
        return data;
    } catch(err) {
        console.log(err);
        throw err;
    }
}

const get4AFCConfig = () => {
    try {
        var data=fs.readFileSync(mediaPath + 'BoothroydCCT.json', 'utf8');
        return data;
    } catch(err) {
        console.log(err);
        throw err;
    }
}

const streamFile = (path, req, contentType, res) => {
    const fileSize = fs.statSync(path).size;
    const range = req.headers.range;
    if (range != null) {
        const boundary = range.replace(/bytes=/, "").split("-");
        const start = parseInt(boundary[0], 10);
        let end = boundary[1] ? parseInt(boundary[1], 10) : start + 999999;
        end = end > fileSize - 1 ? fileSize - 1 : end;
        const chunkSize = (end - start) + 1;
        const file = fs.createReadStream(path, {start, end});
        const head = {
            'Content-Range' : `bytes ${start}-${end}/${fileSize}`,
            'Accept-Ranges' : 'bytes',
            'Content-Length' : chunkSize,
            'Content-Type' : contentType,
        };
        res.writeHead(206, head);
        file.pipe(res);
    }
    else
    {
        let head = { 'Content-Type': contentType };
        res.writeHead(200, head);
        fs.createReadStream(path).pipe(res);
    } 
}

const handleError = (err, res) => {
    if (err.errno == -2) {
        res.status(404).send(err);
    }
    else {
        res.status(500).send(err);
    }
}

module.exports = {
    saltRounds: saltRounds,
    mediaPath,
    videoFilePath,
    audioFilePath,
    mrtFilePath,
    videoExtension,
    audioExtension,
    getFileNames,
    getMRTJSON,
    readJSONFile,
    uploadFiles,
    deleteFile,
    getUserAssessmentConfig,
    get4AFCConfig,
    streamFile,
    handleError
}
