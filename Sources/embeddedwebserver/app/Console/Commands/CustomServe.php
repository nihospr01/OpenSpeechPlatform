<?php

namespace App\Console\Commands;

use Illuminate\Console\Command;
use App\GoldilocksListener;
use App\GoldilocksProgram;
use App\Http\Controllers\ApiController;
use UnexpectedValueException;


class CustomServe extends Command
{
    /**
     * The name and signature of the console command.
     *
     * @var string
     */
    protected $signature = 'app:customserve';

    /**
     * The console command description.
     *
     * @var string
     */
    protected $description = 'Transmit listener program on serve';

    /**
     * Create a new command instance.
     *
     * @return void
     */
    public function __construct()
    {
        parent::__construct();
    }

    /**
     * Execute the console command.
     *
     * @return mixed
     */
    public function handle()
    {
        $listeners = GoldilocksListener::all();
        $curr_listener = GoldilocksListener::where('mostRecent', 1)->first();

       if (is_null($curr_listener) && sizeof($listeners) == 0) {
            $this->call('serve', ['--host' => '0.0.0.0', '--post' => '80']);
            return;
        } else if (is_null($curr_listener) && sizeof($listeners)) {
            $curr_listener = GoldilocksListener::orderByDesc('updated_at')->first();
            $curr_listener->mostRecent = 1;
            $curr_listener->save();
        }

        $program = GoldilocksProgram::where('listener_id', $curr_listener->id)->orderByDesc('updated_at')->first();

        if (is_null($program)) {
            $program = GoldilocksGeneric::first();
        }

        $payload['data'] = $program->parameters;

        $controller = new ApiController;

        try {
            $controller->updateParametersHelper($payload);
        } catch (\Exception $e) {
            error_log($e);
        }

        $this->call('serve', ['--host' => '0.0.0.0', '--post' => '80']);
    }
}
