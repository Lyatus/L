#include "Collider.h"

#include "../gl/GL.h"
#include "../gl/Program.h"
#include "../math/geometry.h"

using namespace L;

void Collider::start() {
  _transform = entity()->requireComponent<Transform>();
  _rigidbody = entity()->component<RigidBody>();
  _script = entity()->component<ScriptComponent>();
  _center = 0.f;
}
void Collider::update() {
  // TODO: replace with broadphase
  Interval3f bb(boundingBox());
  for(auto&& other : Pool<Collider>::global){
    if(&other!=this && other.entity()!=entity() && &other<this && bb.overlaps(other.boundingBox())){
      if(_rigidbody) checkCollision(*this,other); // Rigidbody is always first argument
      else if(other._rigidbody) checkCollision(other,*this);
    }
  }
}
Interval3f Collider::boundingBox() const {
  switch(_type) {
    case Box:
      return Interval3f(_center-_radius,_center+_radius).transformed(_transform->matrix());
    case Sphere:
    {
      Vector3f center(_transform->toAbsolute(_center));
      return Interval3f(center-_radius,center+_radius);
    }
    default:
      return Interval3f();
  }
}
void Collider::render(const Camera& camera) {
  GL::baseProgram().use();
  GL::baseProgram().uniform("model",_transform->matrix()*Matrix44f::scale(_radius));
  GL::cube().draw();
}
Interval1f project(const Vector3f& axis,const Vector3f* points,size_t count){
  if(count--){
    Interval1f wtr(Interval1f(axis.dot(points[count])));
    while(count--)
      wtr.add(axis.dot(points[count]));
    return wtr;
  }
  return Interval1f();
}
Vector3f leastToAxis(const Vector3f& axis,const Vector3f* points,size_t count) {
  if(count--){
    Vector3f wtr(points[count]);
    float leastProjected(axis.dot(points[count]));
    while(count--){
      const Vector3f& p(points[count]);
      float projected(axis.dot(p));
      if(projected<leastProjected) {
        wtr = p;
        leastProjected = projected;
      }
    }
    return wtr;
  }
  return Vector3f();
}
void Collider::checkCollision(Collider& a,Collider& b) {
  if(a._type==Box && b._type==Box) {
    const Transform *at(a._transform),*bt(b._transform);
    const Vector3f ar(at->right()),af(at->forward()),au(at->up()),
      br(bt->right()),bf(bt->forward()),bu(bt->up());
    const Vector3f axes[] = {
      ar,af,au,br,bf,bu,
      ar.cross(br).normalized(),
      ar.cross(bf).normalized(),
      ar.cross(bu).normalized(),
      af.cross(br).normalized(),
      af.cross(bf).normalized(),
      af.cross(bu).normalized(),
      au.cross(br).normalized(),
      au.cross(bf).normalized(),
      au.cross(bu).normalized()
    };
    const Vector3f apoints[] = {
      at->toAbsolute(a._center+Vector3f(-a._radius.x(),-a._radius.y(),-a._radius.z())),
      at->toAbsolute(a._center+Vector3f(-a._radius.x(),-a._radius.y(),a._radius.z())),
      at->toAbsolute(a._center+Vector3f(-a._radius.x(),a._radius.y(),-a._radius.z())),
      at->toAbsolute(a._center+Vector3f(-a._radius.x(),a._radius.y(),a._radius.z())),
      at->toAbsolute(a._center+Vector3f(a._radius.x(),-a._radius.y(),-a._radius.z())),
      at->toAbsolute(a._center+Vector3f(a._radius.x(),-a._radius.y(),a._radius.z())),
      at->toAbsolute(a._center+Vector3f(a._radius.x(),a._radius.y(),-a._radius.z())),
      at->toAbsolute(a._center+Vector3f(a._radius.x(),a._radius.y(),a._radius.z())),
    };
    const Vector3f bpoints[] = {
      bt->toAbsolute(b._center+Vector3f(-b._radius.x(),-b._radius.y(),-b._radius.z())),
      bt->toAbsolute(b._center+Vector3f(-b._radius.x(),-b._radius.y(),b._radius.z())),
      bt->toAbsolute(b._center+Vector3f(-b._radius.x(),b._radius.y(),-b._radius.z())),
      bt->toAbsolute(b._center+Vector3f(-b._radius.x(),b._radius.y(),b._radius.z())),
      bt->toAbsolute(b._center+Vector3f(b._radius.x(),-b._radius.y(),-b._radius.z())),
      bt->toAbsolute(b._center+Vector3f(b._radius.x(),-b._radius.y(),b._radius.z())),
      bt->toAbsolute(b._center+Vector3f(b._radius.x(),b._radius.y(),-b._radius.z())),
      bt->toAbsolute(b._center+Vector3f(b._radius.x(),b._radius.y(),b._radius.z())),
    };
    uintptr_t axis(sizeof(axes));
    float minOverlap;
    Vector3f normal;
    for(uintptr_t i(0); i<sizeof(axes)/sizeof(Vector3f); i++) {
      if(axes[i].lengthSquared()>0.00001f) { // The axis is not a null vector (caused by a cross product)
        Interval1f axisA(project(axes[i],apoints,8)),axisB(project(axes[i],bpoints,8)),intersection(axisA,axisB); // Compute projections and intersection
        float overlap(intersection.size().x());
        if(overlap>0.f) {
          if(axis==sizeof(axes) || overlap<minOverlap) { // First or smallest overlap yet
            normal = (axisA.center().x()<axisB.center().x()) ? -axes[i] : axes[i];
            minOverlap = overlap;
            axis = i;
          }
        } else return; // No overlap means no collision
      }
    }
    // Resolve interpenetration
    if(b._rigidbody){
      a._transform->moveAbsolute(normal*minOverlap*.5f);
      b._transform->moveAbsolute(normal*minOverlap*-.5f);
    } else a._transform->moveAbsolute(normal*minOverlap);
    // Compute impact point
    Vector3f impactPoint(0,0,0);
    if(axis<6)
      impactPoint = (axis<3) ? leastToAxis(-normal,bpoints,8) : leastToAxis(normal,apoints,8);
    else{
      Vector3f avertex(leastToAxis(normal,apoints,8)),bvertex(leastToAxis(-normal,bpoints,8));
      const Vector3f& aaxis(axes[(axis-6)/3]),baxis(axes[((axis-6)%3)+3]);
      if(!lineLineIntersect(avertex,avertex+aaxis,bvertex,bvertex+baxis,&avertex,&bvertex))
        return; // Unable to compute intersection
      impactPoint = (avertex+bvertex)/2.f;
    }
    auto e(ref<Table<Var,Var>>());
    (*e)[FNV1A("type")] = FNV1A("COLLISION");
    if(a._script){
      (*e)[FNV1A("other")] = &b;
      a._script->event(e);
    }
    if(b._script){
      (*e)[FNV1A("other")] = &a;
      b._script->event(e);
    }
    RigidBody::collision(a._rigidbody,b._rigidbody,impactPoint,normal);
  }
}
