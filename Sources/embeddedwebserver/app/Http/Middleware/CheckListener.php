<?php

namespace App\Http\Middleware;

use Closure;
use App\GoldilocksListener;

class CheckListener
{
    /**
     * Handle an incoming request.
     *
     * @param  \Illuminate\Http\Request  $request
     * @param  \Closure  $next
     * @return mixed
     */
    public function handle($request, Closure $next)
    {
        $listeners = GoldilocksListener::all(); 
        $listener = GoldilocksListener::where('mostRecent', 1)->first();
        if (!is_null($listener)) {
            //store in the global session listener and researcher who have logged in
            session(['listener' => $listener->listener]);
            if($listener->researcher_id){
                $researcher = $listener->researcher;
                session(['researcher' => $researcher->researcher]);
            }

            return redirect('/goldilocks/listener/programs');
        } else if (is_null($listener) && sizeof($listeners)) {
            $listener = GoldilocksListener::orderByDesc('updated_at')->first();
            $listener->mostRecent = 1;
            $listener->save();

            session(['listener' => $listener->listener]);
            if($listener->researcher_id){
                $researcher = $listener->researcher;
                session(['researcher' => $researcher->researcher]);
            }

            return redirect('/goldilocks/listener/programs');
        }
        return $next($request);
    }
}
