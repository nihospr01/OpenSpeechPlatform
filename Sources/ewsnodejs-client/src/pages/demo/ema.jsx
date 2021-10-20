import React, { Component } from 'react';
import Button from '@material-ui/core/Button';
import Grid from '@material-ui/core/Grid';
import FormControl from '@material-ui/core/FormControl';
import { FormControlLabel, ListItem } from '@material-ui/core';
import {pages} from './emaPages.json';
import {FormLabel} from '@material-ui/core';
import {Radio, RadioGroup} from '@material-ui/core';
import Checkbox from '@material-ui/core/Checkbox';
import FormGroup from '@material-ui/core/FormGroup';
import LinearProgress from '@material-ui/core/LinearProgress';
import { withStyles } from '@material-ui/core/styles';
import Box from '@material-ui/core/Box';


console.log(pages);
class EMA extends Component {
    
    state = {
        // current page index
        pageIndex: 0,
        progress: 0,

        // arrays corresponding to ema.json which defines page layout
        text: [],
        isQuestion: [],
        isMultiple: [],
        choices: [],
        buttons: [],

        // record user response per page 
        responses: []
    }

    constructor(props) {
        super(props);
        this.loadPages();
    }

    loadPages = () => {
        pages.map( page => {
            this.state.text.push(page.text);
            this.state.isQuestion.push(page.isQuestion);
            this.state.isMultiple.push(page.isMultiple);
            this.state.choices.push(page.choices);
            this.state.buttons.push(page.buttons);
        }
        );
        console.log(this.state.text);
        console.log(this.state.isQuestion);
        console.log(this.state.isMultiple);
        console.log(this.state.choices);
        console.log(this.state.buttons);

        for(var i = 0; i < this.state.isQuestion.length; i++) {
            this.state.responses[i] = [];
        }
    }

    displayText = () => {
        return (
            <Grid>
                <Box m={1} lineHeight={2} fontSize={16} justify="center" align="center">
                    <Box m={1} maxWidth={500} lineHeight={2} fontSize={20} justify="center" align="center">
                    {this.state.text[this.state.pageIndex]}
                </Box>
                </Box>
            </Grid>
            
        )
    }

    displayChoices = () => {
        // if there are no choices to display on this page, do not display
        if (this.state.choices[this.state.pageIndex] == "") {
            return ( <div></div> )
        }

        // display single selection questions as radio buttons
        else if (this.state.isMultiple[this.state.pageIndex] == 0) {
            return (
                <Grid container justifyContent="center" style={{'display':"flex", 'flexDirection':"column"}}>
                    <Box m={1} justify="center" align="center">
                        <FormControl>
                            <FormLabel>Select one:</FormLabel>
                            <RadioGroup onChange={this.answerSelectedRadio}>
                                {this.state.choices[this.state.pageIndex].map((profile, index) => 
                                    <FormControlLabel value={profile} control={<Radio />} label={profile}>{profile}</FormControlLabel>
                                )}
                            </RadioGroup>
                        </FormControl>
                    </Box>                
                </Grid>
            )
        }

        // display multiple selection questions as checkboxes
        else {
            return (
                <Grid container justifyContent="center" style={{'display':"flex", 'flexDirection':"column"}}>                
                    <Box m={1} justify="center" align="center">
                        <FormControl>
                            <FormLabel>Select all that apply:</FormLabel>
                            <FormGroup onChange={this.answerSelectedCheck}>
                                {this.state.choices[this.state.pageIndex].map((profile, index) => 
                                    <FormControlLabel value={profile} control={<Checkbox/>} label={profile}>{profile}</FormControlLabel>
                                )}
                            </FormGroup>
                        </FormControl>
                    </Box>
                </Grid>
            )
        }

    }

    selectButton = () => {
        switch(this.state.buttons[this.state.pageIndex]) {
            case 0:
                return this.displayNext();
                break;
            case 1:
                return this.displayPreviousNext();
                break;
            case 2:
                return this.displaySubmit();
                break;
            default:
                return (<div></div>);
        }   
    }

    displayNext = () => {
        return (
            <div>
                <Grid container justifyContent="center">
                    <Button 
                        variant="contained"
                        onClick={this.nextPage}>Next Page</Button>
                </Grid>
            </div>
        )
    }

    displayPreviousNext = () => {
        
        return (
            <div>
                <Grid container justifyContent="center">
                    <Box p={1}>
                        <Button 
                            variant="contained"
                            onClick={this.previousPage}>
                                Previous Page
                        </Button>
                    </Box>
                    <Box p={1}>
                        <Button 
                            variant="contained"
                            onClick={this.nextPage}>
                                Submit Responses
                        </Button>
                    </Box>
                </Grid>
            </div>
        )
    }

    displaySubmit = () => {
        return (
            <div>
                <Grid container justifyContent="center">
                    <Button 
                        variant="contained"
                        onClick={this.submit}>Submit</Button>
                </Grid>
            </div>
        )
    }

    previousPage = async (event) => {
        // decrement page index
        var dec = this.state.pageIndex - 1;
        this.setState({pageIndex: dec});

        // update progress bar
        var calcProgress = (this.state.pageIndex - 1) * 100/ (this.state.text.length - 1);
        this.setState({progress: calcProgress});
    }

    nextPage = async (event) => {
        // increment page index
        var inc = this.state.pageIndex + 1;
        this.setState({pageIndex: inc});

        // update progress bar
        var calcProgress = (this.state.pageIndex + 1) * 100 / (this.state.text.length - 1);
        this.setState({progress: calcProgress});
    }

    answerSelectedCheck = async (event) => {
        // if the user checks an answer, add it to responses
        if(event.target.checked) {
            this.state.responses[this.state.pageIndex].push(event.target.value);
        }
        // if the user unchecks an answer, remove it from responses
        else {
            const index = this.state.responses[this.state.pageIndex].indexOf(event.target.value);
            if (index > -1) {
                this.state.responses[this.state.pageIndex].splice(index, 1);
            }
        }
        
        console.log(this.state.responses[this.state.pageIndex]);
    }

    answerSelectedRadio = async (event) => {
        this.state.responses[this.state.pageIndex] = [event.target.value];
        console.log(this.state.responses[this.state.pageIndex]);
    }

    // TODO: where do we send responses?    
    submit = async (event) => {
        
    }
    

    render() {
        return (
            <div>
                <div>
                    <LinearProgress variant="determinate" value={this.state.progress} />
                </div>
                <Grid container direction="column" justify="center">
                        {this.displayText()}
                        {this.displayChoices()}
                        {this.selectButton()}
                </Grid>
                
            </div>
        )
    }
}

export default EMA;
