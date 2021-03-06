<?php

/*
|--------------------------------------------------------------------------
| Console Routes
|--------------------------------------------------------------------------
|
| This file is where you may define all of your Closure based console
| commands. Each Closure is bound to a command instance allowing a
| simple approach to interacting with each command's IO methods.
|
*/


/**
 * Command generate:docs. Generates documentation using ApiGen.
 */
Artisan::command('docs:generate', function(){
        //files to be included in the documentation
	    $include = ['app/Http/Controllers/ApiController.php',
            'app/Http/Controllers/Goldilocks/',
            ];

        //construct command
        $command = 'vendor/bin/apigen generate ';
        foreach($include as $file){
            $command = $command.$file.' ';
        }
        $command = $command.' --destination public/apigen';

        //execute
        exec($command);
        echo "\n";

})->describe('Generate documentation using ApiGen.');

/**
 * Delete all of the files generated by ApiGen. ApiGen doesn't offer an option 
 * for doing so.
 */
Artisan::command('docs:clear', function(){
    //construct command
    $command = 'rm ' . getcwd() . "/public/apigen/*.html";

    //execute
    exec($command);

})->describe('Remove documentation generated by ApiGen');