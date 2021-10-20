var express = require('express');
var router = express.Router();
const utils = require('../utils/utils');
const util = require('util');
const exec = util.promisify(require('child_process').exec);
const fs = require('fs');
var net = require('net');
const localIpUrl = require('local-ip-url');

import ResearcherServices from '../services/researcherServices';

var researcherServices = new ResearcherServices();

router.get("/", async(req, res, next) => {
    try {
        const researcherList = await researcherServices.getAllResearchers();
        res.status(200).send(researcherList);
    }
    catch (err) {
        res.status(500).send(err);
    }

})

router.post("/signup", async (req, res, next) => {
    const { researcherID, password } = req.body;
    try {
        const savedResearcher = await researcherServices.signUp(researcherID, password);
        res.status(200).send(savedResearcher);
    }
    catch (err) {
        res.status(500).send(err.message);
    }
})

router.post("/login", async (req, res, next) => {
    const { researcherID, password } = req.body;
    try {
        const researcher = await researcherServices.findUserByID(researcherID);
        if (!researcher) {
            res.status(404).json({
                error: 'User not found'
            });
        }
        else {
            const same =  await researcherServices.validatePassword(researcher, password);

            if (!same) {
                res.status(401).json({
                    error: "Password Incorrect",
                });
            }
            else {
                res.status(200).send(researcher);
            }
        }
    }
    catch (err) {
        res.status(500).json({
            error: 'Server error please try again',
        });
    }
})

router.get("/fileNames", async (req, res, next) => {
    try {

        // const folder_name = req.query.folder_name;
        // TODO: parametraization
        // const fileNames = await utils.getFileNames(folder_name);
        const fileNames = await utils.getFileNames();
        console.log("fileNames="+fileNames);
        // Here we want to return either .wav files or the folder which contains audio files
        // and their transcription. For now we just test suffix for .wav and hard-coded prefix
        // for folders.
        // TODO: the standard way might be checking the type of files
        let audioFileList = [];
        fileNames.forEach(filename => {
                if (filename.endsWith('.wav')) {
                    audioFileList.push({value: filename, transcript: null});
                } else if (filename.startsWith('stims')) {
                    let stimIndex = filename.substring(5);
                    audioFileList.push(
                        {
                            value: filename + '/stim' + stimIndex + '.wav',
                            // value: filename + '/substim0.wav',
                            transcript: filename + '/stim' + stimIndex + '.json'
                        }
                    );
                } 
            }
        );
        console.log("audiofilelist:");
        console.log(audioFileList);
        res.status(200).send(audioFileList);
    }
    catch (err) {
        res.status(500).send(err);
    }
})

// retrieve MRT JSON file names
router.get('/MRTfilename', async (req, res, next) => {

    try {  
        const MRTfilesnames = await utils.getMRTJSON();
        // Here we want to return MRT json files
        // For now we just test prefix for MRT_{}.json and hard-code
        let MRTFileList = [];
        MRTfilesnames.forEach(filename => {
                if (filename.startsWith('MRT_')) {
                    MRTFileList.push({value: filename});
                } 
            }
        );
        console.log("MRTFileList:");
        console.log(MRTFileList);
        res.status(200).send(MRTFileList);
    }
    catch (err) {
        res.status(500).send(err);
    }
})

router.post("/jsonFile", async (req, res) => {
    const filePath = req.body.data;
    console.log("---------------  Start reading from jsonFile --------------"); 
    console.log(filePath);
    console.log(req.body);
    try {
        const jsonData = await utils.readJSONFile(filePath);
        console.log("---------------  Finish reading from jsonFile --------------"); 
        res.status(200).send(jsonData);
    }
    catch (err) {
        res.status(500).send(err);
    }
})


router.get("/audioPath", async (req, res, next) => {
    try {
        const currentPath = utils.audioFilePath;
        res.status(200).send(currentPath);
    }
    catch (err) {
        res.status(500).send(err);
    }
})


// specific to MultiAFC folder
router.get("/mrtFilePath", async (req, res, next) => {
    try {
        const currentFilepath = utils.mrtFilePath;
        console.log("the currentFilepath is  --------------> ",currentFilepath )
        res.status(200).send(currentFilepath);
    }
    catch (err) {
        res.status(500).send(err);
    }
})

router.get("/goldilocksURL", async (req, res, next) => {
    try {
        let goldilocks_URL = "http://" + localIpUrl() + ":8080/goldilocks/listener/login";
        res.status(200).send(goldilocks_URL);
    }
    catch (err) {
        res.status(500).send(err);
    }
})

router.post("/upload", async (req, res, next) => {
    try {
        utils.uploadFiles(req, res);
    }
    catch (err) {
        res.status(500).send(err);
    }
})

router.post("/delete", async (req, res, next) => {
    const { filename } = req.body;
    try {
        utils.deleteFile(filename);
        res.status(200).send("success");
    }
    catch (err) {
        res.status(500).send(err);
    }
})

router.get("/userAssessmentConfig", async (req, res, next) => {
    try {
        const data = utils.getUserAssessmentConfig();
        res.status(200).send(data);
    }
    catch (err) {
        console.log(err)
        res.status(404).send(err);
    }
})

router.get("/4AFCConfig", async (req, res, next) => {
    try {
        const data = utils.get4AFCConfig();
        res.status(200).send(data);
    }
    catch (err) {
        console.log(err)
        res.status(404).send(err);
    }
})
process.on('uncaughtException', error => console.error('Uncaught exception: ', error));
process.on('unhandledRejection', error => console.error('Unhandled rejection: ', error));

// for backwards compatibility.  Deprecated.
router.post("/restartRTMHA", async (req, res, next) => {
    const isTenBand = req.body.data;

    var num_bands = 6;
    if (isTenBand) num_bands = 10;

    var client = new net.Socket();
    client.connect({
        port: 8001,
        host: '127.0.0.1'
    });

    client.on('error', function (err) {
        res.status(500).send("Setting Params Failed. Please make sure MHA is running");
        client.end();
        return;
    });

    client.on('connect', function () {
        client.write(JSON.stringify({ "method": "set", "data": { "num_bands": num_bands } }));
        client.end();
        res.status(200).send("Bands Switched");
    });
})

router.get('/video', async (req, res, next) => {
    const videoPath = utils.videoFilePath + req.query.name;
    try {
        utils.streamFile(videoPath, req, 'video/mp4', res)
    }
    catch (err) {
        console.log(err);
        utils.handleError(err, res);
    }
    
})


router.get('/audio', async (req, res, next) => {
    // const audioPath = utils.mediaPath + "audio/" + req.query.name;
    // utils.mediaPath since multiple APPs may need to access this API
    const audioPath = utils.mediaPath + req.query.name;
    console.log(audioPath);
    try {
        utils.streamFile(audioPath, req, 'audio/wav', res)
    }
    catch (err) {
        console.log(err);
        utils.handleError(err, res);
    }
    
})


module.exports = router;