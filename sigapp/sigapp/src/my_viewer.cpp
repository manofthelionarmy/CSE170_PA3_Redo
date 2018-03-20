
# include "my_viewer.h"

# include <sigogl/ui_button.h>
# include <sigogl/ui_radio_button.h>
# include <sig/sn_primitive.h>
# include <sig/sn_transform.h>
# include <sig/sn_manipulator.h>

# include <sigogl/ws_run.h>
# include <chrono>
# include <ctime>
static bool keeprunning = false;

GsVec lighting = GsVec(0.015f, 0.15f, 0.15f);
GsMat shadowXY = GsMat(1.0f, 0.0f, -lighting.x / lighting.z, 0.0f,
	0.0f, 1.0f, -lighting.y / lighting.z, 0.0f,
	0.0f, 0.0f, 0.0f, 0.0f,
	0.0f, 0.0f, 0.0f, 1.0f);
MyViewer::MyViewer ( int x, int y, int w, int h, const char* l ) : WsViewer(x,y,w,h,l)
{
	_nbut=0;
	_animating=false;
	build_ui ();
	build_scene ();
}

void MyViewer::build_ui ()
{
	UiPanel *p, *sp;
	UiManager* uim = WsWindow::uim();
	p = uim->add_panel ( "", UiPanel::HorizLeft );
	p->add ( new UiButton ( "View", sp=new UiPanel() ) );
	{	UiPanel* p=sp;
		p->add ( _nbut=new UiCheckButton ( "Normals", EvNormals ) ); 
	}
	p->add ( new UiButton ( "Animate", EvAnimate ) );
	p->add ( new UiButton ( "Exit", EvExit ) ); p->top()->separate();
}

void MyViewer::add_model ( SnShape* s, GsVec p )
{
	SnManipulator* manip = new SnManipulator;
	GsMat m;
	m.translation ( p );
	manip->initial_mat ( m );

	SnGroup* g = new SnGroup;
	SnLines* l = new SnLines;
	l->color(GsColor::orange);
	g->add(s);
	g->add(l);
	manip->child(g);
	rootg()->add(manip);
}

void MyViewer::drawClockBody() {
	SnPrimitive* p;
	SnManipulator* manip;
	p = new SnPrimitive(GsPrimitive::Cylinder, 0.5f, 0.5f, 0.01f);
	p->prim().material.diffuse = GsColor::red;
	add_model(p, GsVec(0, 0, 0));

	manip = (SnManipulator *)(rootg()->get(0));

	GsMat &m = manip->mat();
	
}

void MyViewer::drawHourHand() {
	SnPrimitive* p;
	SnManipulator* manip;
	p = new SnPrimitive(GsPrimitive::Capsule, 0.03f, 0.03f, 0.1f);
	p->prim().material.diffuse = GsColor::blue;
	p->prim().center = GsPnt(0.0f, 0.1f, 0.0f);
	add_model(p, GsVec(0, 0, 0));

	manip = (SnManipulator *)(rootg()->get(2));
}
void MyViewer::drawMinuteHand() {
	SnPrimitive* p;
	SnManipulator* manip;
	p = new SnPrimitive(GsPrimitive::Capsule, 0.03f, 0.03f, 0.15f);
	p->prim().material.diffuse = GsColor::green;
	p->prim().center = GsPnt(0.0f, 0.1f, 0.0f);
	add_model(p, GsVec(0, 0, 0));

	manip = (SnManipulator *)(rootg()->get(1));

}
void MyViewer::drawClockShadow() {
	drawClockBody();
	drawMinuteHand();
	drawHourHand();
	GsMat rotMat;
	GsMat translationMat;
	GsMat &clockMat = rootg()->get<SnManipulator>(3)->mat();
	GsMat &minuteH_mat = rootg()->get<SnManipulator>(4)->mat();
	GsMat &hourH_mat = rootg()->get<SnManipulator>(5)->mat();


	translationMat.translation(GsVec(0, -0.5f, -0.5f));

	clockMat.mult(clockMat, translationMat);
	minuteH_mat.mult(minuteH_mat, translationMat);
	hourH_mat.mult(hourH_mat, translationMat);

	rotMat.rotx(GS_TORAD(-90.0f));
	clockMat.mult(clockMat, rotMat);
	minuteH_mat.mult(minuteH_mat, rotMat);
	hourH_mat.mult(hourH_mat, rotMat);

	clockMat.mult(clockMat, shadowXY);
	minuteH_mat.mult(minuteH_mat, shadowXY);
	hourH_mat.mult(hourH_mat, shadowXY);
}
void MyViewer::build_scene ()
{
	drawClockBody();
	drawMinuteHand();
	drawHourHand();

	GsMat &m = rootg()->get<SnManipulator>(0)->mat();
	
	GsMat rotMat;
	rotMat.rotx(GS_TORAD(90.0f));
	m.mult(m, rotMat);

	drawClockShadow();
}

// Below is an example of how to control the main loop of an animation:
void MyViewer::run_animation ()
{
	if ( _animating ) return; // avoid recursive calls
	_animating = true;
	
	int ind = gs_random ( 0, rootg()->size()-1 ); // pick one child
	SnManipulator* manip = rootg()->get<SnManipulator>(ind); // access one of the manipulators
	GsMat m = manip->mat();

	double frdt = 1.0/30.0; // delta time to reach given number of frames per second
	double v = 4; // target velocity is 1 unit per second
	double t=0, lt=0, t0=gs_time();
	do // run for a while:
	{	while ( t-lt<frdt ) { ws_check(); t=gs_time()-t0; } // wait until it is time for next frame
		double yinc = (t-lt)*v;
		if ( t>2 ) yinc=-yinc; // after 2 secs: go down
		lt = t;
		m.e24 += (float)yinc;
		if ( m.e24<0 ) m.e24=0; // make sure it does not go below 0
		manip->initial_mat ( m );
		render(); // notify it needs redraw
		ws_check(); // redraw now
	}	while ( m.e24>0 );
	_animating = false;
}
void MyViewer::clock_animation() {
	if (_animating) return; // avoid recursive calls
	_animating = true;
	
	int minuteHand_index = 1;
	int hourHand_index = 2;
	
	SnManipulator * minuteHand = rootg()->get<SnManipulator>(minuteHand_index);
	SnManipulator * hourHand = rootg()->get<SnManipulator>(hourHand_index);
	GsMat &clockMat = rootg()->get<SnManipulator>(0)->mat();
	GsMat &mat_minuteH = minuteHand->mat();
	GsMat &mat_hourH = hourHand->mat();

	GsMat &shadowMinuteH = rootg()->get<SnManipulator>(4)->mat();
	GsMat &shadowHourH = rootg()->get<SnManipulator>(5)->mat();
	GsMat rotMatrix;

	double frdt = 1.0 / 30.0; // delta time to reach given number of frames per second
	double v = 4; // target velocity is 1 unit per second
	double t = 0, lt = 0, t0 = gs_time();
	float r1dt = 0.0f; 
	float r2dt = 0.0f; 
	
	double start = 0.0;
	double seconds = 0.0;
	do // run for a while:
	{
		while (t - lt<frdt) { ws_check(); t = gs_time() - t0; } // wait until it is time for next frame
		double yinc = (t - lt)*v;
		double end = t;
	
		double elapsed_seconds = end - start;
		if (seconds > 60.0) {

			
			rotMatrix.rotz(GS_TORAD(-6.0f));
			mat_hourH.mult(mat_hourH, rotMatrix);

			shadowHourH.mult(shadowHourH, shadowXY);
			shadowHourH.mult(shadowHourH, rotMatrix);
			seconds = 0.0;
			
		}

		if (elapsed_seconds >= 1.0) {
			seconds += 1.0;
			
			rotMatrix.rotz(GS_TORAD( -6.0f));

			mat_minuteH.mult(mat_minuteH, rotMatrix);

			shadowMinuteH.mult(shadowMinuteH, shadowXY);
			shadowMinuteH.mult(shadowMinuteH, rotMatrix);
			start = end;
		}

		
		

		lt = t;
		
		render(); // notify it needs redraw
		ws_check(); // redraw now
	} while (keeprunning);
	_animating = false;
}
void MyViewer::show_normals ( bool b )
{
	// Note that primitives are only converted to meshes in GsModel
	// at the first draw call.
	GsArray<GsVec> fn;
	SnGroup* r = (SnGroup*)root();
	for ( int k=0; k<r->size(); k++ )
	{	SnManipulator* manip = r->get<SnManipulator>(k);
		SnShape* s = manip->child<SnGroup>()->get<SnShape>(0);
		SnLines* l = manip->child<SnGroup>()->get<SnLines>(1);
		if ( !b ) { l->visible(false); continue; }
		l->visible ( true );
		if ( !l->empty() ) continue; // build only once
		l->init();
		if ( s->instance_name()==SnPrimitive::class_name )
		{	GsModel& m = *((SnModel*)s)->model();
			m.get_normals_per_face ( fn );
			const GsVec* n = fn.pt();
			float f = 0.33f;
			for ( int i=0; i<m.F.size(); i++ )
			{	const GsVec& a=m.V[m.F[i].a]; l->push ( a, a+(*n++)*f );
				const GsVec& b=m.V[m.F[i].b]; l->push ( b, b+(*n++)*f );
				const GsVec& c=m.V[m.F[i].c]; l->push ( c, c+(*n++)*f );
			}
		}  
	}
}

int MyViewer::handle_keyboard ( const GsEvent &e )
{
	int ret = WsViewer::handle_keyboard ( e ); // 1st let system check events
	if ( ret ) return ret;

	switch ( e.key )
	{	case GsEvent::KeyEsc : gs_exit(); return 1;
		case GsEvent::KeySpace: {
			if (keeprunning == false) {
				keeprunning = true;
			}
			else {
				keeprunning = false;
			}
			clock_animation();
			return 1; 
		}
		case GsEvent::KeyEnter:{
			rootg()->remove_all();
			build_scene();
			render();
		}
		case 'n' : { bool b=!_nbut->value(); _nbut->value(b); show_normals(b); return 1; }
		default: gsout<<"Key pressed: "<<e.key<<gsnl;
	}

	return 0;
}

int MyViewer::uievent ( int e )
{
	switch ( e )
	{	case EvNormals: show_normals(_nbut->value()); return 1;
		case EvAnimate: run_animation(); return 1;
		case EvExit: gs_exit();
	}
	return WsViewer::uievent(e);
}
