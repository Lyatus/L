#include "PRS.h"

using namespace L;

PRS::PRS(Point3f position, Point3f rotation, Point3f scale)
: position(position), rotation(rotation), scale(scale){}

void PRS::move(const Point3f& p){
    position.x()+=p.x();
    position.y()+=p.y();
    position.z()+=p.z();
}
void PRS::relMove(const Point3f& p){
    Point3f vx(1.0,0,0), vy(0,1.0,0), vz(0,0,1.0);
    Matrix33f Zrot, Yrot, Xrot;
    float cosZ = cos(rotation.z()*M_PI/180.0),
           sinZ = sin(rotation.z()*M_PI/180.0),
           cosY = cos(rotation.y()*M_PI/180.0),
           sinY = sin(rotation.y()*M_PI/180.0),
           cosX = cos(rotation.x()*M_PI/180.0),
           sinX = sin(rotation.x()*M_PI/180.0);

    Zrot(0,0) = cosZ;   Zrot(1,0) = -sinZ;  Zrot(2,0) = 0;
    Zrot(0,1) = sinZ;   Zrot(1,1) = cosZ;   Zrot(2,1) = 0;
    Zrot(0,2) = 0;      Zrot(1,2) = 0;      Zrot(2,2) = 1;

    Yrot(0,0) = cosY;   Yrot(1,0) = 0;      Yrot(2,0) = sinY;
    Yrot(0,1) = 0;      Yrot(1,1) = 1;      Yrot(2,1) = 0;
    Yrot(0,2) = -sinY;  Yrot(1,2) = 0;      Yrot(2,2) = cosY;

    Xrot(0,0) = 1;      Xrot(1,0) = 0;      Xrot(2,0) = 0;
    Xrot(0,1) = 0;      Xrot(1,1) = cosX;   Xrot(2,1) = -sinX;
    Xrot(0,2) = 0;      Xrot(1,2) = sinX;   Xrot(2,2) = cosX;

    Matrix33f M(Zrot * Yrot * Xrot);

    vx = M * vx;
    vy = M * vy;
    vz = M * vz;

    position.x() += vx.x()*p.x()+vy.x()*p.y()+vz.x()*p.z();
    position.y() += vx.y()*p.x()+vy.y()*p.y()+vz.y()*p.z();
    position.z() += vx.z()*p.x()+vy.z()*p.y()+vz.z()*p.z();
}
void PRS::rotate(const Point<3,float>& p){
    rotation.x() = PMod(rotation.x()+p.x(),360.f);
    rotation.y() = PMod(rotation.y()+p.y(),360.f);
    rotation.z() = PMod(rotation.z()+p.z(),360.f);
}
void PRS::relRotate(const Point<3,float>& p){

}


