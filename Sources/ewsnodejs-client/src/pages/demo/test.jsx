import React, { Component } from 'react';
import { makeStyles } from '@material-ui/styles';
import { withStyles } from '@material-ui/styles';
import Paper from '@material-ui/core/Paper';
import Grid from '@material-ui/core/Grid';
import Button from '@material-ui/core/Button';
import Typography from '@material-ui/core/Typography';
import Box from '@material-ui/core/Box';
import { createMuiTheme, MuiThemeProvider } from '@material-ui/core/styles';
import teal from '@material-ui/core/colors/teal';
import {Link} from 'react-router-dom';
import { compose } from 'recompose';
import {axios} from 'utils/utils';

import SettingsIcon from '@material-ui/icons/Settings';
import DialogTitle from '@material-ui/core/DialogTitle';
import Dialog from '@material-ui/core/Dialog';
import Select from '@material-ui/core/Select';
import MenuItem from '@material-ui/core/MenuItem';

var goldilocks_URL;
const tealTheme = createMuiTheme({ palette: {primary: teal}});

const styles = (theme) => ({
    root: {
      minWidth: 275,
    },
    bullet: {
      display: 'inline-block',
      margin: '0 2px',
      transform: 'scale(0.8)',
    },
    title: {
      fontSize: 14,
    },
    pos: {
      marginBottom: 12,
    },
    gridContainer: {
        paddingLeft: '20px',
        paddingRight: '20px',
    },
    submitButton: {
        width: 200
    },
    settingsBar: {
        display: 'flex',
        flexDirection: 'row',
        justifyContent: 'flex-end',
        width: "100%"
    },
    settingsWindow: {
        width: "100%",
        paddingLeft: 30,
        paddingRight: 30,
        paddingBottom: 30,
        display: 'flex',
        flexDirection: 'column',
        justifyContent: 'flex-start'
    },
    volumeTitle: {
        display: 'flex',
        flexDirection: 'row',
        justifyContent: 'space-between'
    },
    saveSettingsButton: {
        alignSelf: 'center'
    },
    alertWindow: {
        width: 400,
        display: 'flex',
        justifyContent: 'center',
        paddingLeft: 30,
        paddingRight: 30,
        paddingBottom: 20
    },
    switchMsgWindow: {
        display: 'flex',
        flexDirection: 'column',
        alignItems: 'center',
        padding: 20,
    }
  });

class Testing extends Component {
    constructor(props) {
        super(props);

        // update band number and alpha from OSP
        this.getParam().then(console.log)
        .catch(console.error);

        // default state
        this.state = {
            openSettings: false,
            tenBand: false,
            bands: 6,
            cannedAudio: false,
            showSwitchMsg: false,
        };
      }

    /**
     * Getting band number and alpha from OSP
     */
    getParam = async () => {
        try {
            const res = await axios.get("/api/researcher/goldilocksURL");
            goldilocks_URL = res.data;
            const response = await axios.post("/api/param/getparam");
            let data = response.data;
            console.log(Object.values(data));

            // Update band from OSP
            let param_left = data["left"];
            var g50_Left = param_left["g50"];
            var bandNumber = g50_Left.length;
            if(bandNumber == 6){
                this.setState({
                    tenBand: false,
                    bands: 6,
                });
            }
            else if (bandNumber == 10){
                this.setState({
                    tenBand: true,
                    bands: 10,
                });
            }
            else {
                this.setState({
                    bands: 11,
                });
            }

            // Update alpha from OSP
            let stim = param_left["alpha"];
            if(stim == 1){
                this.setState({
                    cannedAudio: true,
                });
            }
            else {
                this.setState({
                    cannedAudio: false,
                });
            }

        } catch (error) {
            console.log(error);
            throw error;
        }
    };

    /**
     * handle the settings page's close.
     */
     handleSettingsClose = () => {
        this.setState({
            openSettings: false
        });

        this.handleSwitchMsgOpen();

    }

    /**
     * Handles the settings page's open
     */
    handleSettingsClick = () => {
        this.setState({
            openSettings: true
        });
    }

    /**
     * Handles the band mode switch
     * @param {*} event event object sent by the select component. Here, event.target.value -- 0 means
     *                  the user is using the 6-band mode, 1 means the user is using the 10-band mode.
     */
    handleBandSwitch = async (event) => {
        // update state, then send new values to OSP
        this.setState({
            tenBand: event.target.value,
            bands: event.target.value,
        }, ()=>{
            this.updateParam();
        });

        try {
            const response = await axios.post("/api/researcher/restartRTMHA", {data: event.target.value});
            console.log(response.data);

            // Show the switch msg to the user.
            this.handleSwitchMsgOpen();
        } catch (error) {
            console.log(error);
        }
    }

        /**
     * Handles the band mode switch
     * @param {*} event event object sent by the select component. Here, event.target.value -- 0 means
     *                  the user is using the 6-band mode, 1 means the user is using the 10-band mode.
     */
    handleStimSwitch = async (event) => {
        // update state, then send new values to OSP
        this.setState({
            cannedAudio: event.target.value,
        }, ()=>{
            this.updateParam();
        });
    
        try {
            const response = await axios.post("/api/researcher/restartRTMHA", {data: event.target.value});
            console.log(response.data);
        } catch (error) {
            console.log(error);
            }
    }

    /**
     * Close the confirmation message window for mode switch.
     */
    handleSwitchMsgClose = () => {
        this.setState({
            showSwitchMsg: false
        });
    }

    /**
     * Open the confirmation message window for mode switch.
     */
    handleSwitchMsgOpen = () => {
        this.setState({
            showSwitchMsg: true
        });
    }

    /**
     * This function loads the HA profile last saved in the database.
     */
     updateParam = async () => {

        // get band number and alpha from state
        var num = this.state.bands;
        var stim = 0;
        if(this.state.cannedAudio){
            stim = 1;
        }

        // send parameters to MHA
        try {
            const response = await axios.post("/api/param/setparam", {
                method: "set",
                data: {
                    num_bands: num,
                    left: {alpha: stim},
                    right: {alpha: stim},
                }
            });
            const data = response.data;
            console.log(data);
        } catch (error) {
            alert(error);
        }
    }


    reset = async() => {
        const dataInputSix ={
            left:{
                alpha: 0.0,
                en_ha:1,
                afc:1,
                freping:1,
                rear_mics:0,
                g50:[0,0,0,0,0,0],
                g80:[0,0,0,0,0,0],
                knee_low:[45,45,45,45,45,45],
                mpo_band:[120,120,120,120,120,120],
                attack:[5,5,5,5,5,5],
                release:[20,20,20,20,20,20],
                global_mpo:120,
                freping_alpha:[0,0,0,0,0.0,0]
            },
            right:{
                en_ha:1,
                afc:1,
                freping:1,
                rear_mics:0,
                g50:[0,0,0,0,0,0],
                g80:[0,0,0,0,0,0],
                knee_low:[45,45,45,45,45,45],
                mpo_band:[120,120,120,120,120,120],
                attack:[5,5,5,5,5,5],
                release:[20,20,20,20,20,20],
                global_mpo:120,
                freping_alpha:[0,0,0,0,0.0,0]
            }
        };
        const dataInputTen ={
            left:{
                alpha: 0.0,
                en_ha:1,
                afc:1,
                freping:1,
                rear_mics:0,
                g50:[0,0,0,0,0,0,0,0,0,0],
                g80:[0,0,0,0,0,0,0,0,0,0],
                knee_low:[45,45,45,45,45,45,45,45,45,45],
                mpo_band:[120,120,120,120,120,120,120,120,120,120],
                attack:[5,5,5,5,5,5,5,5,5,5],
                release:[20,20,20,20,20,20,20,20,20,20],
                global_mpo:120,
                freping_alpha:[0,0,0,0,0,0,0,0,0,0]
            },
            right:{
                alpha: 0.0,
                en_ha:1,
                afc:1,
                freping:1,
                rear_mics:0,
                g50:[0,0,0,0,0,0,0,0,0,0],
                g80:[0,0,0,0,0,0,0,0,0,0],
                knee_low:[45,45,45,45,45,45,45,45,45,45],
                mpo_band:[120,120,120,120,120,120,120,120,120,120],
                attack:[5,5,5,5,5,5,5,5,5,5],
                release:[20,20,20,20,20,20,20,20,20,20],
                global_mpo:120,
                freping_alpha:[0,0,0,0,0,0,0,0,0,0]
            }
        };
        const dataInputEleven ={
            left:{
                alpha: 0.0,
                en_ha:1,
                afc:1,
                freping:1,
                rear_mics:0,
                g50:[0,0,0,0,0,0,0,0,0,0,0],
                g80:[0,0,0,0,0,0,0,0,0,0,0],
                knee_low:[45,45,45,45,45,45,45,45,45,45,45],
                mpo_band:[120,120,120,120,120,120,120,120,120,120,120],
                attack:[5,5,5,5,5,5,5,5,5,5,5],
                release:[20,20,20,20,20,20,20,20,20,20,20],
                global_mpo:120,
                freping_alpha:[0,0,0,0,0,0,0,0,0,0,0]
            },
            right:{
                alpha: 0.0,
                en_ha:1,
                afc:1,
                freping:1,
                rear_mics:0,
                g50:[0,0,0,0,0,0,0,0,0,0,0],
                g80:[0,0,0,0,0,0,0,0,0,0,0],
                knee_low:[45,45,45,45,45,45,45,45,45,45,45],
                mpo_band:[120,120,120,120,120,120,120,120,120,120,120],
                attack:[5,5,5,5,5,5,5,5,5,5,5],
                release:[20,20,20,20,20,20,20,20,20,20,20],
                global_mpo:120,
                freping_alpha:[0,0,0,0,0,0,0,0,0,0,0]
            }
        };
        try {
            if (this.state.bands == 6) {
                const response = await axios.post("/api/param/setparam", {
                    method: 'set',
                    data: dataInputSix,
                });
                const data = response.data;
                console.log(data);
            }
            else if (this.state.bands == 10){
                const response = await axios.post("/api/param/setparam", {
                    method: 'set',
                    data: dataInputTen,
                });
                const data = response.data;
                console.log(data);
            }
            else {
                const response = await axios.post("/api/param/setparam", {
                    method: 'set',
                    data: dataInputEleven,
                });
                const data = response.data;
                console.log(data);
            }
            
        }
        catch(error) {
            alert(error.response.data);
        }
    }

    render() {
        const {classes} = this.props;
        var buttonStyle = {
            display: 'block',
            width: '100%',
            height: '150%',
            fontSize: '15px',
            textTransform: 'none',
        };

        // display these demos for 10 band mode
        if (this.state.tenBand == true) {
            return (
                <div className={classes.root}>
                    <div className={classes.settingsBar} style={{
                        display:"flex",
                        justifyContent:"flex-end",}}>
                            <SettingsIcon className={classes.iconHover} fontSize="large" onClick={this.handleSettingsClick}></SettingsIcon>
                    </div>
                    <div style={{
                        display: "flex",
                        justifyContent: "center",
                        alignContent: "center",
                    }}
                    >
                        <h1 className="display-5"> The Open Speech Platform </h1>
                    </div>
                    
                    <Dialog 
                        open={this.state.openSettings} 
                        onClose={this.handleSettingsClose}>
                            <DialogTitle>Settings</DialogTitle>
                            <Grid container spacing={3}  className={classes.settingsWindow}>
                                <Grid item>
                                    <Typography>Select the number of bands on your device</Typography>
                                    <Select
                                        value={this.state.bands}
                                        onChange={this.handleBandSwitch}
                                        defaultValue={this.state.bands}
                                    >
                                        <MenuItem value={6}>Six Band</MenuItem>
                                        <MenuItem value={10}>Ten Band</MenuItem>
                                    </Select>
                                </Grid>

                                <Grid item>
                                    <Typography>Select live audio or pre-recorded audio</Typography>

                                    <Select
                                        value={this.state.cannedAudio}
                                        onChange={this.handleStimSwitch}
                                    >
                                        <MenuItem value={false}>Live</MenuItem>
                                        <MenuItem value={true}>Pre-Recorded</MenuItem>
                                    </Select>
                                </Grid>
    
                                <Grid item className={classes.saveSettingsButton}>
                                    <Button color='primary' variant="contained" onClick={this.handleSettingsClose}>Done</Button>
                                </Grid>
                            </Grid>
                        </Dialog>
                        <Dialog open={this.state.showSwitchMsg} onClose={this.handleSwitchMsgClose}>
                                        <Grid container className={classes.switchMsgWindow}>
                                            <Typography style={{marginBottom: 10}}>
                                                Now using {this.state.bands} band mode with 
                                                {this.state.cannedAudio ? " pre-recorded" : " live"} audio
                                            </Typography>
                                            <Button onClick={this.handleSwitchMsgClose} color="primary" variant="contained">OK</Button>
                                        </Grid>
                                    </Dialog>
                    <Grid container spacing={5} direction='column'>
                    <Grid item xs={12}>
                        <div style={{
                        display: "flex",
                        justifyContent: "center",
                        alignContent: "center",
                    }}
                    >
                        <h3 className="display-5"> A Real-time, Open, Portable, Extensible Speech Lab. 
                            Visit our <a href={'http://openspeechplatform.ucsd.edu/'}>website</a> to learn more.</h3>
                    </div>
                        </Grid>
                        <Grid container spacing={10} direction='column'>
                        <Grid item xs={12}>
                            <Grid container spacing={10} className={classes.gridContainer} >
                                <Grid item xs={12} md={6} lg={4}>
                                    <MuiThemeProvider theme={tealTheme}>
                                        <Link to={'/main/researcherpage'}>
                                            <Button variant='outlined' color='primary' style={buttonStyle} variant='contained'>
                                                <Typography component="div">
                                                    <Box textAlign="center" fontWeight="fontWeightBold" fontSize={20}>
                                                        Researcher Page
                                                    </Box>
                                                    <Box textAlign="center" m={1}>
                                                        Includes amplification, noise and feedback parameters.
                                                    </Box>
                                                </Typography>
                                            </Button>
                                        </Link>
                                    </MuiThemeProvider>
                                </Grid>
                                
                                <Grid item item item xs={12} md={6} lg={4}>
                                    <MuiThemeProvider theme={tealTheme}>
                                        <Link to={'/main/beamformingDemo'}>
                                            <Button variant='outlined' color='primary' style={buttonStyle} variant='contained'>
                                                <Typography component="div">
                                                    <Box textAlign="center" fontWeight="fontWeightBold" fontSize={20}>
                                                        Beamforming Task
                                                    </Box>
                                                    <Box textAlign="center">
                                                        Manipulation of the parameters associated with OSP’s adaptive beamformer
                                                    </Box>
                                                </Typography>
                                            </Button>
                                        </Link>
                                    </MuiThemeProvider>
                                </Grid>
                                <Grid item item item xs={12} md={6} lg={4}>
                                        <MuiThemeProvider theme={tealTheme}>
                                            <Link to={'/main/FourAFCDemo'}>
                                                <Button variant='outlined' color='primary' style={buttonStyle} variant='contained'>
                                                    <Typography component="div">
                                                        <Box textAlign="center" fontWeight="fontWeightBold" fontSize={20}>
                                                            Multi-Alternative Forced Choice (Multi-AFC) Task
                                                        </Box>
                                                        <Box textAlign="center" >
                                                            Mobile-assisted word recognition tests using minimal contrast sets.
                                                        </Box>
                                                    </Typography>
                                                </Button>
                                            </Link>
                                        </MuiThemeProvider>
                                    </Grid>
                            </Grid>
                        </Grid>
                        <Grid item xs={12}>
                            <Grid container spacing={10} className={classes.gridContainer} >
                                    
                                    <Grid item item xs={12} md={6} lg={4}>
                                    <MuiThemeProvider theme={tealTheme}>
                                        <Link to={'/main/freppingDemo'}>
                                            <Button variant='outlined' color='primary' style={buttonStyle} variant='contained'>
                                                <Typography component="div">
                                                    <Box textAlign="center" fontWeight="fontWeightBold" fontSize={20}>
                                                        Freping
                                                    </Box>
                                                    <Box textAlign="center" m={1}>
                                                        Includes amplification, noise and feedback parameters.
                                                    </Box>
                                                </Typography>
                                            </Button>
                                        </Link>
                                    </MuiThemeProvider>
                                </Grid>
                                    <Grid item item item xs={12} md={6} lg={4}>
                                        <MuiThemeProvider theme={tealTheme}>
                                            <Link to={'/main/reset'}>
                                                <Button onClick={this.reset} variant='outlined' color='primary' style={buttonStyle} variant='contained'>
                                                    <Typography component="div">
                                                        <Box textAlign="center" fontWeight="fontWeightBold" fontSize={20}>
                                                            Profile Selection
                                                        </Box>
                                                        <Box textAlign="center" >
                                                            Reset to a Prescription Profile
                                                        </Box>
                                                    </Typography>
                                                </Button>
                                            </Link>
                                        </MuiThemeProvider>
                                    </Grid>
                            </Grid>
                            </Grid>
                        </Grid>
                    </Grid>
                </div>
            );
        } 
        // display these demos for 6 band mode
        else {
            return (
                <div className={classes.root}>
                    <div className={classes.settingsBar} style={{
                        display:"flex",
                        justifyContent:"flex-end",}}>
                            <SettingsIcon className={classes.iconHover} fontSize="large" onClick={this.handleSettingsClick}></SettingsIcon>
                    </div>
                    <div style={{
                        display: "flex",
                        justifyContent: "center",
                        alignContent: "center",
                    }}
                    >
                        <h1 className="display-5"> The Open Speech Platform </h1>
                    </div>
                    
                    
                    <Dialog open={this.state.openSettings} onClose={this.handleSettingsClose}>
                            <DialogTitle>Settings</DialogTitle>
                            <Grid container spacing={3}  className={classes.settingsWindow}>
                            <Grid item>
                                    <Typography>Select the number of bands on your device</Typography>
                                    <Select
                                        value={this.state.bands}
                                        onChange={this.handleBandSwitch}
                                        defaultValue={this.state.bands}
                                    >
                                        <MenuItem value={6}>Six Band</MenuItem>
                                        <MenuItem value={10}>Ten Band</MenuItem>
                                    </Select>
                                </Grid>

                                <Grid item>
                                    <Typography>Select live audio or pre-recorded audio</Typography>

                                    <Select
                                        value={this.state.cannedAudio}
                                        onChange={this.handleStimSwitch}
                                    >
                                        <MenuItem value={false}>Live</MenuItem>
                                        <MenuItem value={true}>Pre-Recorded</MenuItem>
                                    </Select>
                                </Grid>
    
                                <Grid item className={classes.saveSettingsButton}>
                                    <Button color='primary' variant="contained" onClick={this.handleSettingsClose}>Done</Button>
                                </Grid>
                            </Grid>
                        </Dialog>

                        <Dialog open={this.state.showSwitchMsg} onClose={this.handleSwitchMsgClose}>
                                        <Grid container className={classes.switchMsgWindow}>
                                            <Typography style={{marginBottom: 10}}>
                                                Now using {this.state.bands} band mode with 
                                                {this.state.cannedAudio ? " pre-recorded" : " live"} audio
                                            </Typography>
                                            <Button onClick={this.handleSwitchMsgClose} color="primary" variant="contained">OK</Button>
                                        </Grid>
                        </Dialog>
                    <Grid container spacing={5} direction='column'>
                        <Grid item>
                        <div style={{
                        display: "flex",
                        justifyContent: "center",
                        alignContent: "center",
                    }}
                    >
                        <h3 className="display-5"> A Real-time, Open, Portable, Extensible Speech Lab. 
                            Visit our <a href={'http://openspeechplatform.ucsd.edu/'}>website</a> to learn more.</h3>
                    </div>
                        </Grid>
                        <Grid container spacing={10} direction='column'>
                        <Grid item xs={12}>
                            <Grid container spacing={10} className={classes.gridContainer} >
                                <Grid item xs={12} md={6} lg={4}>
                                    <MuiThemeProvider theme={tealTheme}>
                                        <Link to={'/main/researcherpage'}>
                                            <Button variant='outlined' color='primary' style={buttonStyle} variant='contained'>
                                                <Typography component="div">
                                                    <Box textAlign="center" fontWeight="fontWeightBold" fontSize={20}>
                                                        Researcher Page
                                                    </Box>
                                                    <Box textAlign="center" m={1}>
                                                        Includes amplification, noise and feedback parameters.
                                                    </Box>
                                                </Typography>
                                            </Button>
                                        </Link>
                                    </MuiThemeProvider>
                                </Grid>
                                <Grid item item item xs={12} md={6} lg={4}>
                                    <MuiThemeProvider theme={tealTheme}>
                                        <Link to={'/main/beamformingDemo'}>
                                            <Button variant='outlined' color='primary' style={buttonStyle} variant='contained'>
                                                <Typography component="div">
                                                    <Box textAlign="center" fontWeight="fontWeightBold" fontSize={20}>
                                                        Beamforming Task
                                                    </Box>
                                                    <Box textAlign="center">
                                                        Manipulation of the parameters associated with OSP’s adaptive beamformer
                                                    </Box>
                                                </Typography>
                                            </Button>
                                        </Link>
                                    </MuiThemeProvider>
                                </Grid>
                                <Grid item item item xs={12} md={6} lg={4}>
                                        <MuiThemeProvider theme={tealTheme}>
                                            <Link to={'/main/FourAFCDemo'}>
                                                <Button variant='outlined' color='primary' style={buttonStyle} variant='contained'>
                                                    <Typography component="div">
                                                        <Box textAlign="center" fontWeight="fontWeightBold" fontSize={20}>
                                                            Multi-Alternative Forced Choice (Multi-AFC) Task
                                                        </Box>
                                                        <Box textAlign="center" >
                                                            Mobile-assisted word recognition tests using minimal contrast sets.
                                                        </Box>
                                                    </Typography>
                                                </Button>
                                            </Link>
                                        </MuiThemeProvider>
                                    </Grid>
                            </Grid>
                        </Grid>
                        <Grid item xs={12}>
                            <Grid container spacing={10} className={classes.gridContainer} >
                                    <Grid item item item xs={12} md={6} lg={4}>
                                        <MuiThemeProvider theme={tealTheme}>
                                            <Link to={'/main/coarseFitDemo'}>
                                                <Button variant='outlined' color='primary' style={buttonStyle} variant='contained'>
                                                    <Typography component="div">
                                                        <Box textAlign="center" fontWeight="fontWeightBold" fontSize={20}>
                                                            CoarseFit Task
                                                        </Box>
                                                        <Box textAlign="center" >
                                                            Includes amplification, noise and feedback parameters.
                                                        </Box>
                                                    </Typography>
                                                </Button>
                                            </Link>
                                        </MuiThemeProvider>
                                    </Grid>
                                    <Grid item item item xs={12} md={6} lg={4}>
                                        <MuiThemeProvider theme={tealTheme}>
                                            <a href= {goldilocks_URL}>
                                                <Button variant='outlined' color='primary' style={buttonStyle} variant='contained'>
                                                    <Typography component="div">
                                                        <Box textAlign="center" fontWeight="fontWeightBold" fontSize={20}>
                                                            Goldilocks Listener
                                                        </Box>
                                                        <Box textAlign="center" >
                                                            Includes researcher interface and end user interface
                                                        </Box>
                                                    </Typography>
                                                </Button>
                                            </a>   
                                        </MuiThemeProvider>
                                    </Grid>

                                <Grid item item xs={12} md={6} lg={4}>
                                    <MuiThemeProvider theme={tealTheme}>
                                        <Link to={'/main/freppingDemo'}>
                                            <Button variant='outlined' color='primary' style={buttonStyle} variant='contained'>
                                                <Typography component="div">
                                                    <Box textAlign="center" fontWeight="fontWeightBold" fontSize={20}>
                                                        Freping
                                                    </Box>
                                                    <Box textAlign="center" m={1}>
                                                        Includes amplification, noise and feedback parameters.
                                                    </Box>
                                                </Typography>
                                            </Button>
                                        </Link>
                                    </MuiThemeProvider>
                                </Grid>
                            </Grid>
                        </Grid>
                        <Grid item xs={12}>
                            <Grid container spacing={10} className={classes.gridContainer} >
                                    <Grid item item item xs={12} md={6} lg={4}>
                                        <MuiThemeProvider theme={tealTheme}>
                                            <Link to={'/main/reset'}>
                                                <Button variant='outlined' color='primary' style={buttonStyle} variant='contained'>
                                                    <Typography component="div">
                                                        <Box textAlign="center" fontWeight="fontWeightBold" fontSize={20}>
                                                            Profile Selection
                                                        </Box>
                                                        <Box textAlign="center" >
                                                            Reset to a Prescription Profile
                                                        </Box>
                                                    </Typography>
                                                </Button>
                                            </Link>
                                        </MuiThemeProvider>
                                    </Grid>
                            </Grid>
                        </Grid>
                        </Grid>
                        
                    </Grid>
                </div>
            );
        }
        
    }
}

export default compose(
    withStyles(styles)
)
(Testing);

