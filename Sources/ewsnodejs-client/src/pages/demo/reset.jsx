import React, { Component } from 'react';
import Button from '@material-ui/core/Button';
import Grid from '@material-ui/core/Grid';
import { axios } from 'utils/utils';
import { FlashOnTwoTone } from '@material-ui/icons';
import Select from '@material-ui/core/Select';
import MenuItem from '@material-ui/core/MenuItem';
import Typography from '@material-ui/core/Typography';
import FormControl from '@material-ui/core/FormControl';
import InputLabel from '@material-ui/core/InputLabel';

var bandNumber;
var profiles = [''];

class Reset extends Component {
    state = {
        profile: 0,
        profileArray: [],
        cannedAudio: false
    }

    constructor(props) {

        super(props);
        this.getParam();
        this.getProfiles();
    }


    handleProfileSwitch = async (event) => {
        // update state
        this.setState({
            profile: event.target.value,
        }, () => {
            this.reset();
        });
    }

    reset = async () => {
        //
        console.log("reset");


        profiles.map(async (profile, index) => {
            if(this.state.profile == index) {
                try {
                    var url = "/api/db/profiles/" + profile;
                    const profileResponse = await axios.get(url);
                    const profileData = profileResponse.data;
                    
                    console.log(profileData);

                    const postResponse = await axios.post("/api/param/setparam", {
                        method: 'set',
                        data: profileData,
                    });
                    const responseData = postResponse.data;

                    console.log(responseData);
                }
                catch (error) {
                    alert(error.response.data);
                }
            }
        });

            console.log("cannedAudio: ");
            console.log(this.state.cannedAudio);

                // get alpha from state
                var stim = 0;
                if(this.state.cannedAudio){
                    stim = 1;
                }

               // send parameters to MHA
               try {
                const alphaResponse = await axios.post("/api/param/setparam", {
                    method: "set",
                    data: {
                        left: {alpha: stim},
                        right: {alpha: stim},
                    }
                });
                const alphaData = alphaResponse.data;
                console.log("alphaData");
                console.log(alphaData);
            } catch (error) {
                alert(error);
            }

    }

    getProfiles = async () => {
        try {
            
            const response = await axios.get("/api/db/profiles");
            const data = response.data;

            console.log("Profiles: ");
            console.log(response.data);
            profiles = data;
    }   catch (error) {
            alert(error.response.data);
        }
    }


   /**
     * Getting band number and alpha from OSP
     */
    getParam = async () => {
        try {
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

    async componentDidMount() {
        try {
            await this.getProfiles();
            
        }
        catch (error) {
            throw error;
        }
        console.log("mount");
        this.setState({profileArray: profiles});
    }

    render() {
        return (
            <div onLoad={this.getProfiles}>
                <Grid container justify="center">
                    <Grid item>
                        <Typography>Discard changes made to hearing aid settings and select a profile to load</Typography>
                        <FormControl>
                            <Select
                                displayEmpty
                                onChange={this.handleProfileSwitch}
                                defaultValue={""}
                            >
                                <MenuItem value="" disabled>
                                    Keep current values
                                </MenuItem>
                                   
                                {this.state.profileArray.map((profile, index) => 
                                <MenuItem key={index} value={index}> {profile}</MenuItem>   // map profiles 
                                )}
                            </Select>
                        </FormControl>
                    </Grid>
                </Grid>

            </div>
        )
    }
}

export default Reset;
