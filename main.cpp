#include <SDL3/SDL.h>
#include <SDL3_image/SDL_image.h>
#include <windows.h>//用于移动控制台窗口
#include<iostream>
#include<sstream>
#include <vector>
#include <cmath>
#include<string>
#include<algorithm>
// 常量
const int WINDOW_WIDTH = 1080;//窗口宽度
const int WINDOW_HEIGHT = 607;//窗口高度
const Uint64 TARGET_FRAME_TIME = 1000000000ULL / 60;//锁定六十帧








int INIT() {//初始化函数
	if (!SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO)) {
		throw std::string("sdl初始化失败");
		return -1;
	};

	return 0;
}

class Entity {//基类，负责渲染控制
protected:
	double pos_x, pos_y;//坐标（贴图左上角）
	int width, height;//大小
	const char* path;//贴图路径

	SDL_Texture *texture;//建立纹理
	SDL_Renderer* render = nullptr;//渲染器指针（初始化为空）
	SDL_FRect rect;//定义位置
	Entity(const Entity&) = delete;
	Entity& operator=(const Entity&) = delete;
public:
	int index = 0;//渲染优先级
	Entity(double x, double y,int w,int h, const char* Path,SDL_Renderer *renderer,int i=0):width(w),height(h),path(Path),index(i) {
		render = renderer;
		pos_x = x; pos_y = y; texture = IMG_LoadTexture(render, path);
		if (!texture) {
			SDL_Log("IMG_LoadTexture 失败，SDL错误: %s", SDL_GetError());
		}
		rect.x = pos_x;
		rect.y = pos_y;
		rect.w = width;
		rect.h = height;
	}

	virtual void Draw() {//渲染贴图
		SDL_RenderTexture(render, texture, NULL, &rect);
	}
	~Entity() {
		SDL_DestroyTexture(texture);
	}
};
bool SortForRender(Entity* e1, Entity* e2) {
	return e1->index < e2->index;
}
class objects :public Entity {//objects类，主要用来负责控制贴图运动
protected:
		double X, Y;//都是球心的坐标
	double radiu;
	double V_x=0, V_y=0;
public:

	objects(double R, double x, double y, int w, int h, const char* Path, SDL_Renderer* renderer, int i = 0) :Entity(x, y, w, h, Path, renderer,i)
		,radiu(R),X(x+R),Y(y+R){
	}
	virtual void pos_update() {
		X += V_x;
		Y += V_y;
		rect.x = X-radiu;
		rect.y = Y-radiu;
	}
	virtual void  Draw() override {
		SDL_RenderTexture(render, texture, NULL, &rect);
	}
	double getX() {
		return X;
	}
	double getY() {
		return Y;
	}
};
class Cue_stick:public objects {//球杆类
protected:
	double angle=0;
	
	Balls* white_ball;
public:
	Cue_stick(Balls*wb, double R,double x,double y,int w,int h,const char*Path,SDL_Renderer* renderer,int i=0):objects(R,x,y,w,h,Path,renderer,i){
		white_ball = wb;
	}
	bool isRender = true;
	void Draw() override {
		if(isRender)
		SDL_RenderTextureRotated(
			render, texture, NULL, &rect, angle, NULL, SDL_FLIP_NONE
		);
		
	}
	void pos_update() override {
		float* m_x;
		float* m_y;
		SDL_GetMouseState(m_x, m_y);
		double dist = sqrt((*m_x - (white_ball->getX())) * (*m_x - (white_ball->getX())) + (*m_y - (white_ball->getY())) * (*m_y - (white_ball->getY())));
		double e[2] = { (*m_x - white_ball->getX())/dist,(*m_y - white_ball->getY())/dist };
		angle = atan2(e[0], e[1]);
		std::cout << angle;

	}
	
};
class Spin {//旋转类，只考虑上下旋和侧旋，不考虑扎杆的左右旋
private:
	const static double FOLLOW_factor;
	const static double SIDE_factor;
	
public:
	double X = 0;
	double Y = 0;
	double side = 0;
	Spin(double x=0,double y=0,double s=0) {
		X = x; Y = y;
		side = s;
	}
};
class Balls :public objects {//balls类用来管理物理碰撞与摩擦
private:
	Spin spin;
	 const static double E;//碰撞系数，0-1之间
	 const static double A_s;//滑动加速度
	 const static double A_r;//滚动加速度
public:
	Balls(double R, double x, double y, int w, int h, const char* Path, SDL_Renderer* renderer, int i = 1) :
		objects(R, x, y, w, h, Path, renderer, i) {
	}
	bool Isitcollided(Balls* ball) {//碰撞检测
		double dist = (X - ball->X) * (X - ball->X) + (Y - ball->Y) * (Y - ball->Y);
		if (dist == 0)return true;
		//double V[2] = { V_x,V_y };//本球的速度向量
		//double V_b[2] = { ball->V_x,ball->V_y };//目标球的速度向量
		double en[2] = { (ball->X - X) /sqrt( dist),(ball->Y - Y) / sqrt(dist) };//两球单位法向量
		//if (((((V[0] * en[0] + V[1] * en[1]) * en[0]) - ((V_b[0] * en[0] + V_b[1] * en[1]) * en[0]))*en[0]
		//	+ (((V[0] * en[0] + V[1] * en[1]) * en[1]) - (V_b[0] * en[0] + V_b[1] * en[1]) * en[1])*en[1]) < 0)
		//	return false;
		double rel_vn = (V_x - ball->V_x) * en[0] + (V_y - ball->V_y) * en[1];
		if (rel_vn < 0) return false;
		if (1156 < dist) {
			return false;
		}
		return true;
	}
	void Collision_Simulate(Balls *ball) {
		/*当前版本只是简单计算碰撞，不考虑旋转*/
		//法向速度n,切向速度t
		double V_t0, V_n0, V_t1, V_n1;
		double dist = sqrt((X - ball->X) * (X - ball->X) + (Y - ball->Y) * (Y - ball->Y));
		if (dist == 0) {
			return;
		}
		double V[2] = { V_x,V_y };//本球的速度向量
		double V_b[2] = { ball->V_x,ball->V_y };//目标球的速度向量
		double en[2] = { (ball->X - X) / dist,(ball->Y - Y) / dist };//两球单位法向量
		double et[2] = { -en[1],en[0] };//两球单位切向量
		//切向速度（暂时）不变，法向速度动量守恒
		double V_t[2] = { (V[0] * et[0]   +  V[1] * et[1])*et[0],(V[0] * et[0] + V[1] * et[1]) * et[1] };
		double V_t_b[2] = { (V_b[0] * et[0] + V_b[1] * et[1]) * et[0],(V_b[0] * et[0] + V_b[1] * et[1]) * et[1] };
		//(V[0] * en[0] + V[1] * en[1])* en[0]
		//(V[0] * en[0] + V[1] * en[1])* en[1]
		//(V_b[0] * en[0] + V_b[1] * en[1])* en[0]
		//(V_b[0] * en[0] + V_b[1] * en[1])* en[1]
		double V_n[2] = { ((V[0] * en[0] + V[1] * en[1]) * en[0] + (V_b[0] * en[0] + V_b[1] * en[1]) * en[0]
			- E * ((V[0] * en[0] + V[1] * en[1]) * en[0] - (V_b[0] * en[0] + V_b[1] * en[1]) * en[0])) / 2,
			((V[0] * en[0] + V[1] * en[1]) * en[1] + (V_b[0] * en[0] + V_b[1] * en[1]) * en[1]
			- E * ((V[0] * en[0] + V[1] * en[1]) * en[1] - (V_b[0] * en[0] + V_b[1] * en[1]) * en[1])) / 2 };
		double V_n_b[2] = {((V[0] * en[0] + V[1] * en[1]) * en[0] + (V_b[0] * en[0] + V_b[1] * en[1]) * en[0]
			+ E * ((V[0] * en[0] + V[1] * en[1]) * en[0] - (V_b[0] * en[0] + V_b[1] * en[1]) * en[0])) / 2,
			((V[0] * en[0] + V[1] * en[1]) * en[1] + (V_b[0] * en[0] + V_b[1] * en[1]) * en[1]
				+ E * ((V[0] * en[0] + V[1] * en[1]) * en[1] - (V_b[0] * en[0] + V_b[1] * en[1]) * en[1])) / 2 };

		//更新速度
		V_x = V_n[0] + V_t[0];
		V_y = V_n[1] + V_t[1];
		ball->V_x = V_n_b[0] + V_t_b[0];
		ball->V_y = V_n_b[1] + V_t_b[1];
	}
	void Set_speed(double vx, double vy) {
		V_x = vx; V_y = vy;
	}

	void Side_simulate() {
		//上边
		if (Y < 123 + radiu&&V_y<0&&191<X&&X<887) {
			V_y = -V_y * E;
		}
		//下边
		if (Y > 479 - radiu&&V_y>0 && 191 < X && X < 887) {
			V_y = -V_y * E;
		}
		//左边
		if (X < 156 + radiu&&V_x<0&&Y>157&&Y<445) {
			V_x = -V_x * E;
		}
		//右边
		if (X > 924 - radiu&&V_x>0 && Y > 157 && Y < 445) {
			V_x = -V_x * E;
		}
		//只实现了边界反弹，还有袋口
	}
	void New_Side_simulate(std::vector<int**> lines) {//可以设置任意直线边界(单侧)碰撞模拟
		for (auto line : lines) {
			double d = sqrt((line[0][0] - line[1][0]) * (line[0][0] - line[1][0]) + (line[0][1] - line[1][1]) * (line[0][1] - line[1][1]));
			double et[2] = { (line[1][0] - line[0][0]) / d,(line[1][1] - line[0][1]) / d };
			double en[2] = { et[1],-et[0] };//法向量指向线段外侧(球所在的一侧)
			double dist = abs((line[1][0] - line[0][0]) * (Y - line[0][1]) - (line[1][1] - line[0][1]) * (X - line[0][0])) / d;
			double vn = V_x * en[0] + V_y * en[1];
			double t = (X - line[0][0]) * et[0] + (Y - line[0][1]) * et[1];
			if (dist < radiu && vn>0&&0<t&&t<d) {
				double vt = V_x * et[0] + V_y * et[1];
				vn = -vn*E;
				V_x = vn * en[0] + et[0] * vt;
				V_y= vn * en[1] + et[1] * vt;
			}
		}
	}
	void friction() {
		double V_R = sqrt(V_x * V_x + V_y * V_y);
		double V_r[2] = { V_x - spin.X,V_y - spin.Y };
		if (abs(V_r[0]) < A_r && abs(V_r[1]) < A_r) {//没有相对滑动，开始滚动计算(需要同步更新球速与转速)
			spin.X = V_x;//同步滚动速度
			spin.Y = V_y;
			if (V_R > A_r) {//大于一帧的速度变化量，-=ar
				V_x -= A_r * (V_x / V_R);
				V_y -= A_r * (V_y / V_R);
				spin.X-= A_r * (V_x / V_R);
				spin.Y-= A_r * (V_y / V_R);
			}
			else {//小于一帧的速度变化量，直接置0
				spin.X = 0;
				spin.Y = 0;
				V_x = 0;
				V_y = 0;
			}
		}
		else {//存在相对滑动
			/*需要计算相对滑动矢量方向，求加速度的方向，给出旋转与滑动的速度变化*/
			double vr = sqrt(V_r[0] * V_r[0] + V_r[1] * V_r[1]);
			V_x -= min(A_s,vr) * (V_r[0] / vr);
			V_y -= min(A_s, vr) * (V_r[1] / vr);
			spin.X+= min(A_s, vr) * (V_r[0] / vr);
			spin.Y+= min(A_s, vr) * (V_r[1] / vr);
		}
	}//带滚动的摩擦模拟完成！！！
	//还有库边碰撞，复位，落袋

};
const double Balls::E = 0.85;
const double Balls::A_s = 0.08;
const double Balls::A_r = 0.03;
//写入边界

void Get_Lines(std::vector<int**>&lines, std::string Points) {
	std::istringstream input_points(Points);
	int Point_A_x, Point_A_y;
	int Point_B_x, Point_B_y;
	int count = 0;
	while (input_points >> Point_A_x >> Point_A_y >> Point_B_x >> Point_B_y) {
		int* PointA;
		int *PointB;
		PointA = new int[2];
		PointA[0] = Point_A_x;
		PointA[1] = Point_A_y;
		PointB = new int[2];
		PointB[0] = Point_B_x;
		PointB[1] = Point_B_y;
		int** line = new int* [2];
		line[0] = PointA;
		line[1] = PointB;
		lines.push_back(line);
		count++;
	}
	std::clog << "成功写入" << count << "线段";
}

void COLLISION_calculate(std::vector<Balls*> ball_set,std::vector<int**>lines) {
	for (int i = 0; i < ball_set.size()-1;i++) {
		for (int j = i + 1; j < ball_set.size(); j++) {
			if (ball_set[i]->Isitcollided(ball_set[j])) {
				ball_set[i]->Collision_Simulate(ball_set[j]);
			}
		}
	}
	for (int i = 0; i < ball_set.size(); i++) {
		ball_set[i]->New_Side_simulate(lines);//测试新的边界碰撞
		ball_set[i]->friction();
	}

}

void print_Mouse_state() {//鼠标位置展示
	float x, y;
	SDL_GetMouseState(&x, &y);
	std::cout << "\r( " << x << " , " << y << " )\033[K"<<std::flush;
}

int main() {
	//把控制台黑框缩小并移到屏幕左上角，避免挡住游戏窗口
	HWND hConsole = GetConsoleWindow();
	if (hConsole) {
		MoveWindow(hConsole, 0, 0, 480, 300, TRUE);
	}
	INIT();//初始化
	SDL_Window* mainwin;//初始化窗口
	SDL_Renderer* renderer;//渲染器初始化
	SDL_CreateWindowAndRenderer("test", WINDOW_WIDTH, WINDOW_HEIGHT, SDL_WINDOW_RESIZABLE, &mainwin, &renderer);
	bool isrunning = true;
	SDL_Event events;//事件初始化
	//加载资源
	Entity test(0., 0., 1080, 607, "resources/picture/background.png", renderer,0);
	Balls ball0(17, 500, 320, 35, 35, "resources/picture/ball0.png", renderer, 1);
	Balls ball1(17, 700, 320, 35, 35, "resources/picture/ball1.png", renderer, 1);
	Cue_stick stick(&ball0,0, 350, 300, 546, 30, "resources/picture/cue_stick.png", renderer, 2);
	if (!SDL_SetRenderVSync(renderer, 1)) {
		SDL_Log("开启垂直同步失败: %s", SDL_GetError());
	}
	std::vector<int**>lines;//载入所有碰撞边界
	std::string a = "173 106 ";
	std::string b = "191 124 ";
	std::string c = "510 124 ";
	std::string d = "516 106 ";
	std::string e = "564 106 ";
	std::string f = "572 125 ";
	std::string g = "889 125 ";
	std::string h = "907 106 ";
	std::string i = "943 140 ";
	std::string j = "924 158 ";
	std::string k = "924 446 ";
	std::string l = "943 462 ";
	std::string m = "907 499 ";
	std::string n = "889 479 ";
	std::string o = "572 479 ";
	std::string p = "564 499 ";
	std::string q = "516 499 ";
	std::string r = "510 479 ";
	std::string s = "191 479 ";
	std::string t = "173 499 ";
	std::string u = "136 462 ";
	std::string w = "191 446 ";
	std::string x = "191 158 ";
	std::string y = "136 140 ";
	Get_Lines(lines,a+b+b+c+c+d );
	Get_Lines(lines,e+f+f+g+g+h );
	Get_Lines(lines,i+j+j+k+k+l );
	Get_Lines(lines,m+n+n+o+o+p );
	Get_Lines(lines,q+r+r+s+s+t );
	Get_Lines(lines,u+w+w+x+x+y );

	std::vector<objects*> objs;//object容器，用于迭代计算运动用于渲染
	std::vector<Entity*> resources;//entity容器，用来渲染
	std::vector<Balls*> ball_set;
	
	ball_set.push_back(&ball0);
	ball_set.push_back(&ball1);

	resources.push_back(&test);
	resources.push_back(&ball0);
	resources.push_back(&ball1);
	resources.push_back(&stick);
	std::sort(resources.begin(), resources.end(), SortForRender);

	objs.push_back(&ball0);
	objs.push_back(&ball1);
	


	ball0.Set_speed(10, 0);
	//




	while (isrunning) {
		Uint64 frame_start = SDL_GetTicksNS();//帧数控制开始计时
		SDL_RenderClear(renderer);
		//事件处理
		while (SDL_PollEvent(&events)) {
			switch (events.type) {
			case SDL_EVENT_QUIT:
				isrunning = false;
			}
		}
		//debug用,打印鼠标坐标
		print_Mouse_state();

		//物理模拟
		COLLISION_calculate(ball_set,lines);


		//计算运动
		for (int i = 0; i < objs.size(); i++) {
			objs[i]->pos_update();
		}
		//渲染内容
		for (int i = 0; i < resources.size(); i++) {
			resources[i]->Draw();
		}
		
		SDL_RenderPresent(renderer);
		//计算当前帧时间，等待到目标时间
		Uint64 elasped = SDL_GetTicksNS() - frame_start;
		if (elasped < TARGET_FRAME_TIME) {
			SDL_DelayPrecise(TARGET_FRAME_TIME - elasped);
		}
		
	}
	SDL_DestroyWindow(mainwin);
	SDL_DestroyRenderer(renderer);
	SDL_Quit();
}