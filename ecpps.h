#ifndef ECS_H
#define ECS_H

#include <vector>
#include <string>
#include <map>
#include <set>
#include <iostream>
#include <typeinfo>
#include <memory>
#include <cstdarg>

using std::vector;
using std::string;
using std::map;
using std::set;
using std::type_info;
using std::is_base_of;
using std::unique_ptr;

// going to try to keep this constrained to one file
// the goal of this system is to provide an OOP-style interface
// while maintaining DDP-style performance undernearth

namespace ecpps {
typedef unsigned ID;
class ECSManager;
class ComponentManager;

// ####### Class definitions ####### //

// wrapper class, entity is really just an ID but this gives OOP approach to management. serves as a reference point for components
class Entity {
    protected:
        // reference to manager for when adding component
        ECSManager& manager;
        // unique ID for every entity
        ID entityID;
    public:
        // constructor
        Entity(ID entityID, ECSManager* manager) : entityID(entityID),manager(*manager){ init(); };
        // meant to be override to allow subclasses to easily add components to self
        virtual void init(){};
        // adds component to manager with entity id
        template <typename T> inline void addComponent(T component);
        // return's entity id
        inline ID getID();
        // used to destroy object for all component managers and delete self from manager
        inline void destroy();
};

// base struct for components, meant to be dervied and then added to entities
struct Component {
};

// base struct for components designed for rendering
struct RenderComponent : public Component {
};

// class for maintaining component vector and entity indexes
class IComponentVector {
    private:
    public:
        virtual void removeEntity(ID entityID)=0;
};

// class for maintaining component vector and entity indexes
template <typename T>
class ComponentVector : public IComponentVector {
    private:
        // holds indexes of entityID
        map<ID, unsigned> indexes;
        // holds vector of components
        vector<T> components;
        // holds a list of entities
        set<ID> entities;
        // holds a seperate list of entities to init
        set<ID> newEntities;
        // private function for getting pointer
        vector<T>& getComponentVector() { return components; };
    public:
        inline void addComponent(ID entityID, T component);
        inline set<ID>& getComponentEntities();
        inline set<ID>& getNewComponentEntities();
        inline void groupEntities();
        inline void removeEntity(ID entityID) override;
        inline T& getComponent(ID entityID);
};

// manages component vectors and tosses around pointers like it's nothing
class ComponentManager {
    private:
        map<const char*, std::shared_ptr<IComponentVector>> componentVectors;
    public:
        template <typename T> void addComponent(ID entityID, T component);
        template <typename T> inline set<ID>& getComponentEntities();
        template <typename T> inline set<ID>& getNewComponentEntities();
        template <typename T> inline void groupEntities();
        template <typename T> inline T& getComponent(ID entityID);
        template <typename T> std::shared_ptr<ComponentVector<T>> getComponentVector();
        inline void removeEntity(ID entityID);
};

// note: for the systems there are two of each function
// depending on what kind of data you need
// base class for systems, fed vectors of components and then perform operations on them
class System {
    private:
    public:
        virtual void init() {};
        virtual void init(ECSManager* manager) { init(); };
        virtual void update() {};
        virtual void update(ECSManager* manager) { update(); };
};

// base class for systems used for rendering
class RenderSystem : public System {
    private:
    public:
        virtual void render() {};
        virtual void render(ECSManager* manager) { render(); };
};

// holds much of the top level ECS data and functionality
class ECSManager {
    private:
        // holds the id for this manager
        ID managerID;
        // holds all systems for looping through updates
        vector<unique_ptr<System>> systems;
        // holds all render systems for looping through renders
        vector<unique_ptr<RenderSystem>> rsystems;
        // holds all component vectors
        ComponentManager components;
        // holds all Entity class objects (must be kept around until ready to delete)
        map<ID, Entity> entities;
        // holds special entities, obtainable by name
        map<string, ID&> specialEntities;
        // holds current value to generate new entityIDs
        ID nextID = 0;
        // holds all IDs that once belonged to entities that have been deleted (reusable)
        vector<ID> reusableIDs;
        // creates a unique ID for each enitity
        inline ID generateEntityID();
    public:
        inline ECSManager(); 
        // creates a default entity
        inline Entity& createEntity();
        // creates an entity of T subclass
        template <typename T, typename... Args> inline Entity& createEntity(Args... args);
        // destroys an entity
        inline void destroyEntity(ID entityID);
        // used to save unique entity in map
        inline void setSpecialEntity(string entityName, Entity& entity);
        // used to retreive unique entity
        inline ID& getSpecialEntity(string entityName);
        // adds a component of any type to a database of T (subclass of component)
        template <typename T> inline void addComponent(ID entityID, T component);
        // adds a component of any type to a database of T (subclass of component) and entityID of ECSmanager
        template <typename T> inline void addComponent(T component);
        // gets a set of all relevant entities per component
        template <typename T> inline set<ID>& getComponentEntities();
        // gets a set of all entity/components ready to init
        template <typename T> inline set<ID>& getNewComponentEntities();
        // used to move init components back into regular pool
        template <typename T> inline void groupEntities();
        // gets a component of type and entity
        template <typename T> inline T& getComponent(ID entityID);
        // gets a component of any type and entityID of ECSmanager itself
        template <typename T> inline T& getComponent();
        // registers a new system
        template <typename T> inline void registerSystem();
        // inits all systems
        inline virtual void init();
        // updates all systems
        inline virtual void update();
        // renders all rendersystems
        inline virtual void render();
};

// ####### Everything else ####### //







// ------- Entity ------- //

template <typename T>
void Entity::addComponent(T component){
    // check and see if object is derived from Component
    if(is_base_of<Component,T>::value == 1){
        // send component to manager
        manager.addComponent<T>(entityID, component);
    }
}

ID Entity::getID(){
    // return id
    return entityID;
}

void Entity::destroy(){
    // tell manager to destroy enemy
    manager.destroyEntity(entityID);
}

// ------- ComponentManager ------- //

template <typename T>
void ComponentVector<T>::addComponent(ID entityID, T component) {
    // define index
    unsigned index;
    // if empty
    if(components.empty()){
        // set to 0
       index = 0;
        // else if populated
    } else {
        // get vector size (will be the same as getting it later and subtracting 1)
        index = components.size();
    }
    
    // get entity index in internal map
    indexes.insert({entityID, index});
    // add entity to init set
    newEntities.emplace(entityID);
    // place component in vector
    components.emplace_back(component);
    
    std::cout << " -------- adding component to id: " << entityID << std::endl;
    std::cout << "typename: " << typeid(T).name() << std::endl;
    std::cout << "current entity: " << entityID << " at index: " << index << std::endl;
    std::cout << "existing component indexes" << std::endl;
    for (const auto& [key, value]: indexes) {
        std::cout << "key: " << key << ", value: " << value << std::endl;
    }
}

template <typename T>
void ComponentVector<T>::removeEntity(ID entityID) {
    // get index of entity
    unsigned index = indexes.at(entityID);
    // remove index from map
    indexes.erase(entityID);
    // remove entity from vector
    components.erase(components.begin() + index);
    // update all indexes
    std::cout << " -------- removing entity with id: " << entityID << std::endl;
    std::cout << "typename: " << typeid(T).name() << std::endl;
    std::cout << "current entity: " << entityID << " at index: " << index << std::endl;
    std::cout << "changing all other component indexes" << std::endl;
    for (const auto& [key, value]: indexes) {
        std::cout << "key: " << key << ", value: " << value << std::endl;
        // if stored index of other entity is higher than the index of the deleted entity
        if(value > index){
            // then subtract by 1
            indexes[key] = value - 1;
            std::cout << "key: " << key << ", new value: " << value << std::endl;
        } else {
            std::cout << "retained old value" << std::endl;
        }
    }
}

template <typename T>
inline T& ComponentVector<T>::getComponent(ID entityID) {
    // get index of entity
    unsigned index = indexes[entityID];
    // return component at index
    return components.at(index);
}

template <typename T>
set<ID>& ComponentVector<T>::getComponentEntities(){
    return entities;
}

template <typename T>
set<ID>& ComponentVector<T>::getNewComponentEntities(){
    return newEntities;
}

template <typename T>
void ComponentVector<T>::groupEntities(){
    // push init group into regular group
    entities.insert(newEntities.begin(), newEntities.end());
    // clear init group
    newEntities.clear();
}

template <typename T>
std::shared_ptr<ComponentVector<T>> ComponentManager::getComponentVector(){
    // first, get type_info to check
    const char* typekey = typeid(T).name();

    // check if map entry exists
    if(componentVectors.find(typekey) == componentVectors.end()){
        //if not, create one
        componentVectors.insert({typekey, std::make_shared<ComponentVector<T>>()});
    }

    // return pointer for component vector
    return std::static_pointer_cast<ComponentVector<T>>(componentVectors.at(typekey));
}

template <typename T>
void ComponentManager::addComponent(ID entityID, T component){
    // this is where the pointer-magic happens
    // get pointer for type, then send new component data
    getComponentVector<T>()->addComponent(entityID, component);
}

template <typename T>
inline T& ComponentManager::getComponent(ID entityID) {
    return getComponentVector<T>()->getComponent(entityID);
}

template <typename T>
set<ID>& ComponentManager::getComponentEntities(){
    return getComponentVector<T>()->getComponentEntities();
}

template <typename T>
set<ID>& ComponentManager::getNewComponentEntities(){
    return getComponentVector<T>()->getNewComponentEntities();
}

template <typename T>
void ComponentManager::groupEntities(){
    return getComponentVector<T>()->groupEntities();
}

void ComponentManager::removeEntity(ID entityID){
    for(const auto& [key, value] : componentVectors){
        auto& component = value;
        
        component -> removeEntity(entityID);
    }
}

// ------- ECSManager ------- //

ECSManager::ECSManager(){
    // add entity id for self
    Entity& thisEntity = createEntity();
    // set self id to entity id
    managerID = thisEntity.getID();
}

template <typename T, typename... Args>
Entity& ECSManager::createEntity(Args... args){
    // check and see if T is derived from Entity
    if(is_base_of<Entity,T>::value == 1){
        // get unique ID
        ID newID = generateEntityID();
        // create entity with id and reference to manager
        T entity(newID, this, args...);

        std::cout << " -------- creating entity at id: " << newID << std::endl;

        // add entity to vector
        entities.insert({newID, entity});
        // return reference to entity
        return entities.at(newID);
    }
}

// constructor for entity, technically
Entity& ECSManager::createEntity(){
    // create generic entity
    return createEntity<Entity>();
}

// destroys an entity
void ECSManager::destroyEntity(ID entityID) {
    // get entity from map
    Entity& toDestroy = entities.at(entityID);

    std::cout << " -------- deleting entity at id: " << entityID << std::endl;

    // remove entity from components
    components.removeEntity(entityID);
    // erase entity from id map
    entities.erase(entityID);
    // add entity id to reclaimable id list
    reusableIDs.emplace_back(entityID);
}

inline void ECSManager::setSpecialEntity(string entityName, Entity& entity){
    // insert hehe (thought it was more complicated)
    ID entityID = entity.getID();

    specialEntities.insert({entityName, entityID});

}

inline ID& ECSManager::getSpecialEntity(string entityName){
    // find value and return
    // if in map
    if(specialEntities.find(entityName) != specialEntities.end()){
        // return entity
        return specialEntities.at(entityName);
        // else
    } else {
        throw "error: no special entity";
    }
}

template <typename T>
void ECSManager::addComponent(ID entityID, T component){ 
    // check and see if object is derived from Component
    if(is_base_of<Component,T>::value == 1){
        // pass to component manager
        components.addComponent<T>(entityID, component);
    }
}

template <typename T>
void ECSManager::addComponent(T component){ 
    addComponent<T>(managerID, component);
}

template <typename T>
set<ID>& ECSManager::getComponentEntities(){
    // check and see if object is derived from Component
    if(is_base_of<Component,T>::value == 1){
        // pass to component manager
        return components.getComponentEntities<T>();
    }
}

template <typename T>
set<ID>& ECSManager::getNewComponentEntities(){
    return components.getNewComponentEntities<T>();
}

template <typename T>
void ECSManager::groupEntities(){
    return components.groupEntities<T>();
}

template <typename T>
inline T& ECSManager::getComponent(ID entityID) {
    return components.getComponent<T>(entityID);
}

template <typename T>
inline T& ECSManager::getComponent() {
    return getComponent<T>(managerID);
}

template <typename T>
void ECSManager::registerSystem(){
    // check if system
    if constexpr (is_base_of<System,T>::value == 1){
        std::cout << typeid(T).name() << std::endl;
        // create system
        unique_ptr<T> system = std::make_unique<T>();
        // next, add to vector
        // check if render system
        if constexpr (is_base_of<RenderSystem,T>::value == 1){
            // if so, add to render systems
            rsystems.emplace_back(std::move(system));
            // init
            rsystems.back()->init(this);
        } else {
            // otherwise, add to systems
            systems.emplace_back(std::move(system));
            // init
            systems.back()->init(this);
        }
    }
}

ID ECSManager::generateEntityID(){
    // prepare ID for return
    ID toReturn;
    // if available reusable ids
    if(reusableIDs.size() != 0){
        // pull the one from the end
        toReturn = reusableIDs.back();
        // remove the one from the end
        reusableIDs.pop_back();
    // if no reusable ids
    } else {
        // return current id pointer
        toReturn = nextID;
        // iterate id pointer
        nextID++;
    }
    //return ID
    return toReturn;
}

void ECSManager::init(){
    // update all systems
    for(unique_ptr<System>& system : systems){
        system->init(this);
    }
    // update all render systems
    for(unique_ptr<RenderSystem>& rsystem : rsystems){
        rsystem->init(this);
    }
}

void ECSManager::update(){
    // update all systems
    for(unique_ptr<System>& system : systems){
        system->update(this);
    }
    // update all render systems
    for(unique_ptr<RenderSystem>& rsystem : rsystems){
        rsystem->update(this);
    }
}

void ECSManager::render(){
    // render all render systems
    for(unique_ptr<RenderSystem>& rsystem : rsystems){
        rsystem->render(this);
    }
}

};
#endif // ECS_H